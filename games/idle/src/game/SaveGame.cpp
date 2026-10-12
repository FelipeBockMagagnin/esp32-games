#include "SaveGame.h"
#include <Preferences.h>

static const char *const NAMESPACE = "idlegame";
// Two slots written in turn, each one key with its checksum inside, so a power cut during
// a write can only lose that write: the other slot still holds the previous save
static const char *const KEY_SLOTS[2] = {"save0", "save1"};
// The single-slot format before that, with the checksum under its own key; read once
// so an existing save survives the change, and removed by the first slot write
static const char *const KEY_LEGACY_BLOB = "blob";
static const char *const KEY_LEGACY_CRC = "crc";

struct StoredSave
{
    SaveBlob blob;
    uint32_t sequence; // Higher is newer
    uint32_t crc;      // Over everything before it
};

// Static rather than on the loop task's stack; only one save or load runs at a time
static StoredSave stored;
static uint32_t lastSequence = 0;
// Always the slot other than the one the live save came from, so a torn write never
// takes the last good copy with it
static uint8_t nextSlot = 0;
static bool legacyPresent = false;

// FNV-1a: tiny, and plenty to tell a torn or stale write from a good one
static uint32_t checksum(const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

// Changes whenever a config table gains or loses a row, which shifts every snapshot
// after it, even when SAVE_VERSION was not bumped
static uint32_t layoutFingerprint()
{
    const uint32_t sizes[] = {
        (uint32_t)sizeof(SaveBlob), BUILDING_COUNT, GOLD_UPGRADE_COUNT, ITEM_COUNT,
        EQUIP_SLOT_COUNT, ZONE_COUNT, ACHIEVEMENT_COUNT, FEATURE_COUNT, TALENT_COUNT};
    return checksum(sizes, sizeof(sizes));
}

void captureSave(SaveBlob &blob, const GameState &game, const Inventory &inventory,
                 const CombatState &combat, const Achievements &achievements, const Unlocks &unlocks,
                 const Ascension &ascension)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    game.save(blob.game);
    inventory.save(blob.inventory);
    combat.save(blob.combat);
    achievements.save(blob.achievements);
    unlocks.save(blob.unlocks);
    ascension.save(blob.ascension);
}

void clearRun(SaveBlob &blob)
{
    memset(&blob.game, 0, sizeof(blob.game));
    memset(&blob.combat, 0, sizeof(blob.combat));
}

void applySave(const SaveBlob &blob, GameState &game, Inventory &inventory, CombatState &combat,
               Achievements &achievements, Unlocks &unlocks, Ascension &ascension)
{
    ascension.load(blob.ascension);
    inventory.load(blob.inventory);
    achievements.load(blob.achievements);
    game.load(blob.game);
    unlocks.load(blob.unlocks);
    combat.load(blob.combat);
}

static bool isUsable(const SaveBlob &blob)
{
    return blob.version == SAVE_VERSION && blob.layout == layoutFingerprint();
}

bool saveGame(const SaveBlob &blob)
{
    stored.blob = blob;
    stored.sequence = lastSequence + 1;
    stored.crc = checksum(&stored, offsetof(StoredSave, crc));

    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false))
    {
        return false;
    }
    bool ok = prefs.putBytes(KEY_SLOTS[nextSlot], &stored, sizeof(stored)) == sizeof(stored);
    if (ok)
    {
        lastSequence = stored.sequence;
        nextSlot ^= 1;
        if (legacyPresent)
        {
            prefs.remove(KEY_LEGACY_BLOB);
            prefs.remove(KEY_LEGACY_CRC);
            legacyPresent = false;
        }
    }
    prefs.end();
    return ok;
}

bool loadGame(SaveBlob &blob)
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, true))
    {
        return false; // Namespace missing: nothing was ever saved
    }

    bool found = false;
    uint32_t foundSequence = 0;
    for (uint8_t slot = 0; slot < 2; slot++)
    {
        const char *key = KEY_SLOTS[slot];
        if (!prefs.isKey(key) || prefs.getBytesLength(key) != sizeof(stored) ||
            prefs.getBytes(key, &stored, sizeof(stored)) != sizeof(stored) ||
            stored.crc != checksum(&stored, offsetof(StoredSave, crc)))
        {
            continue;
        }
        // Counts on past a slot this firmware cannot read, so the next write is the newest
        if (stored.sequence > lastSequence)
        {
            lastSequence = stored.sequence;
        }
        if (isUsable(stored.blob) && (!found || stored.sequence > foundSequence))
        {
            blob = stored.blob;
            found = true;
            foundSequence = stored.sequence;
            nextSlot = slot ^ 1;
        }
    }

    legacyPresent = prefs.isKey(KEY_LEGACY_BLOB);
    if (!found && legacyPresent)
    {
        found = prefs.getBytesLength(KEY_LEGACY_BLOB) == sizeof(blob) &&
                prefs.getBytes(KEY_LEGACY_BLOB, &blob, sizeof(blob)) == sizeof(blob) &&
                prefs.getUInt(KEY_LEGACY_CRC, 0) == checksum(&blob, sizeof(blob)) && isUsable(blob);
    }
    prefs.end();
    return found;
}

void eraseSave()
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.clear();
        prefs.end();
    }
    lastSequence = 0;
    nextSlot = 0;
    legacyPresent = false;
}

static const unsigned long SAVE_INTERVAL_MS = 60000;
static const unsigned long SAVE_SOON_MS = 5000;

AutoSaver::AutoSaver(const GameState &game, const Inventory &inventory, const CombatState &combat,
                     const Achievements &achievements, const Unlocks &unlocks, const Ascension &ascension)
    : game(game),
      inventory(inventory),
      combat(combat),
      achievements(achievements),
      unlocks(unlocks),
      ascension(ascension),
      buffer(),
      lastSaved(),
      nextSaveAt(0),
      savedMilestones(0)
{
}

void AutoSaver::begin(unsigned long now, const SaveBlob *loaded)
{
    if (loaded != nullptr)
    {
        lastSaved = *loaded;
    }
    savedMilestones = milestoneSignature();
    nextSaveAt = now + SAVE_INTERVAL_MS;
}

// Folds the slow-moving progress counters into one number, so a change in any of them
// can be noticed without comparing the whole blob every loop
uint32_t AutoSaver::milestoneSignature() const
{
    uint32_t sig = inventory.getRevision();
    sig = sig * 31 + game.getTotalBuildingLevels();
    sig = sig * 31 + game.getUpgradesBoughtCount();
    sig = sig * 31 + game.getMiningLevel();
    sig = sig * 31 + achievements.getUnlockedCount();
    sig = sig * 31 + unlocks.getRevision();
    sig = sig * 31 + ascension.getRevision();
    sig = sig * 31 + (combat.isInZone() ? combat.getCurrentZone() + 1 : 0);
    return sig;
}

void AutoSaver::update(unsigned long now)
{
    uint32_t milestones = milestoneSignature();
    if (milestones != savedMilestones && (long)(nextSaveAt - (now + SAVE_SOON_MS)) > 0)
    {
        nextSaveAt = now + SAVE_SOON_MS;
    }
    if ((long)(now - nextSaveAt) < 0)
    {
        return;
    }

    nextSaveAt = now + SAVE_INTERVAL_MS;
    savedMilestones = milestones;
    captureSave(buffer, game, inventory, combat, achievements, unlocks, ascension);
    // Nothing moved since the last write (no production yet): spare the flash
    if (memcmp(&buffer, &lastSaved, sizeof(SaveBlob)) == 0)
    {
        return;
    }
    if (saveGame(buffer))
    {
        lastSaved = buffer;
    }
    else
    {
        Serial.println("Save failed");
    }
}
