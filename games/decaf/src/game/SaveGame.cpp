#include "SaveGame.h"
#include <Preferences.h>

static const char *const NAMESPACE = "decaf";
// Two slots written in turn, each one key with its checksum inside, so a power cut during
// a write can only lose that write: the other slot still holds the previous save
static const char *const KEY_SLOTS[2] = {"save0", "save1"};

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
    const uint32_t sizes[] = {(uint32_t)sizeof(SaveBlob), LEVEL_COUNT};
    return checksum(sizes, sizeof(sizes));
}

void newSave(SaveBlob &blob)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
}

bool recordClear(SaveBlob &blob, uint8_t level, uint32_t ticks, uint16_t deaths)
{
    if (level >= LEVEL_COUNT)
    {
        return false;
    }
    FloorRecord &r = blob.floors[level];
    bool first = r.clears == 0;
    if (r.clears < UINT16_MAX)
    {
        r.clears++;
    }
    if (first || deaths < r.fewestDeaths)
    {
        r.fewestDeaths = deaths;
    }
    if (first || ticks < r.bestTicks)
    {
        r.bestTicks = ticks;
        return true;
    }
    return false;
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
        bool readable = stored.blob.version == SAVE_VERSION && stored.blob.layout == layoutFingerprint();
        if (readable && (!found || stored.sequence > foundSequence))
        {
            blob = stored.blob;
            found = true;
            foundSequence = stored.sequence;
            nextSlot = slot ^ 1;
        }
    }

    prefs.end();
    return found;
}

static const char *const KEY_SETTINGS = "settings";

bool loadSettings(Settings &out)
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, true))
    {
        return false;
    }
    size_t stored = prefs.getBytesLength(KEY_SETTINGS);
    bool ok = stored > 0 && stored <= sizeof(out);
    if (ok)
    {
        Settings read = {};
        ok = prefs.getBytes(KEY_SETTINGS, &read, stored) == stored;
        if (ok)
        {
            out = read;
        }
    }
    prefs.end();
    return ok;
}

void saveSettings(const Settings &in)
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.putBytes(KEY_SETTINGS, &in, sizeof(in));
        prefs.end();
    }
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
}
