#include "Storage.h"
#include <Preferences.h>

static const char *const NAMESPACE = "pomodoro";
static const char *const KEY_SETTINGS = "settings";
static const char *const KEY_SLOTS[2] = {"stats0", "stats1"};

struct StoredStats
{
    FocusStats::Snapshot stats;
    uint16_t version;
    uint32_t sequence; // Higher is newer
    uint32_t crc;      // Over everything before it
};

static uint32_t lastSequence = 0;
// Always the slot other than the one the live stats came from, so a torn write never takes
// the last good copy with it
static uint8_t nextSlot = 0;

// FNV-1a
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

bool loadStats(FocusStats::Snapshot &out)
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, true))
    {
        return false;
    }
    bool found = false;
    uint32_t foundSequence = 0;
    StoredStats stored;
    for (uint8_t slot = 0; slot < 2; slot++)
    {
        const char *key = KEY_SLOTS[slot];
        if (!prefs.isKey(key) || prefs.getBytesLength(key) != sizeof(stored) ||
            prefs.getBytes(key, &stored, sizeof(stored)) != sizeof(stored) ||
            stored.crc != checksum(&stored, offsetof(StoredStats, crc)))
        {
            continue;
        }
        if (stored.sequence > lastSequence)
        {
            lastSequence = stored.sequence;
        }
        if (stored.version == STATS_VERSION && (!found || stored.sequence > foundSequence))
        {
            out = stored.stats;
            found = true;
            foundSequence = stored.sequence;
            nextSlot = slot ^ 1;
        }
    }
    prefs.end();
    return found;
}

bool saveStats(const FocusStats::Snapshot &in)
{
    // Zeroed first so the padding the checksum covers is stable
    StoredStats stored;
    memset(&stored, 0, sizeof(stored));
    stored.stats = in;
    stored.version = STATS_VERSION;
    stored.sequence = lastSequence + 1;
    stored.crc = checksum(&stored, offsetof(StoredStats, crc));

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

void eraseStats()
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.remove(KEY_SLOTS[0]);
        prefs.remove(KEY_SLOTS[1]);
        prefs.end();
    }
    lastSequence = 0;
    nextSlot = 0;
}
