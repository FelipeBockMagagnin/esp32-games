#include "SaveGame.h"
#include <Preferences.h>

static const char *const NAMESPACE = "idlegame";
static const char *const KEY_BLOB = "blob";
static const char *const KEY_CRC = "crc";

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
        EQUIP_SLOT_COUNT, ZONE_COUNT, ACHIEVEMENT_COUNT, FEATURE_COUNT};
    return checksum(sizes, sizeof(sizes));
}

void captureSave(SaveBlob &blob, const GameState &game, const Inventory &inventory,
                 const CombatState &combat, const Achievements &achievements, const Unlocks &unlocks)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    game.save(blob.game);
    inventory.save(blob.inventory);
    combat.save(blob.combat);
    achievements.save(blob.achievements);
    unlocks.save(blob.unlocks);
}

void applySave(const SaveBlob &blob, GameState &game, Inventory &inventory, CombatState &combat,
               Achievements &achievements, Unlocks &unlocks)
{
    inventory.load(blob.inventory);
    achievements.load(blob.achievements);
    game.load(blob.game);
    unlocks.load(blob.unlocks);
    combat.load(blob.combat);
}

bool saveGame(const SaveBlob &blob)
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false))
    {
        return false;
    }
    bool ok = prefs.putBytes(KEY_BLOB, &blob, sizeof(blob)) == sizeof(blob) &&
              prefs.putUInt(KEY_CRC, checksum(&blob, sizeof(blob))) == sizeof(uint32_t);
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
    bool ok = prefs.getBytesLength(KEY_BLOB) == sizeof(blob) &&
              prefs.getBytes(KEY_BLOB, &blob, sizeof(blob)) == sizeof(blob) &&
              prefs.getUInt(KEY_CRC, 0) == checksum(&blob, sizeof(blob));
    prefs.end();
    return ok && blob.version == SAVE_VERSION && blob.layout == layoutFingerprint();
}

void eraseSave()
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.clear();
        prefs.end();
    }
}
