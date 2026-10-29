#include "SlotStore.h"
#include <Preferences.h>

static const char *const KEY_SLOTS[2] = {"save0", "save1"};
static const size_t TRAILER = 2 * sizeof(uint32_t); // Sequence, then the checksum over everything before it

// Static rather than on the loop task's stack; only one save or load runs at a time
static uint8_t buffer[SlotStore::MAX_SIZE + TRAILER];

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

bool SlotStore::save(const void *data, size_t len)
{
    if (len > MAX_SIZE)
    {
        return false;
    }
    uint32_t sequence = lastSequence + 1;
    memcpy(buffer, data, len);
    memcpy(buffer + len, &sequence, sizeof(sequence));
    uint32_t crc = checksum(buffer, len + sizeof(sequence));
    memcpy(buffer + len + sizeof(sequence), &crc, sizeof(crc));

    Preferences prefs;
    if (!prefs.begin(nameSpace, false))
    {
        return false;
    }
    bool ok = prefs.putBytes(KEY_SLOTS[nextSlot], buffer, len + TRAILER) == len + TRAILER;
    prefs.end();
    if (ok)
    {
        lastSequence = sequence;
        nextSlot ^= 1;
    }
    return ok;
}

bool SlotStore::load(void *data, size_t len)
{
    if (len > MAX_SIZE)
    {
        return false;
    }
    Preferences prefs;
    if (!prefs.begin(nameSpace, true))
    {
        return false; // Namespace missing: nothing was ever saved
    }

    bool found = false;
    uint32_t foundSequence = 0;
    for (uint8_t slot = 0; slot < 2; slot++)
    {
        const char *key = KEY_SLOTS[slot];
        if (!prefs.isKey(key) || prefs.getBytesLength(key) != len + TRAILER ||
            prefs.getBytes(key, buffer, len + TRAILER) != len + TRAILER)
        {
            continue;
        }
        uint32_t sequence, crc;
        memcpy(&sequence, buffer + len, sizeof(sequence));
        memcpy(&crc, buffer + len + sizeof(sequence), sizeof(crc));
        if (crc != checksum(buffer, len + sizeof(sequence)))
        {
            continue;
        }
        if (sequence > lastSequence)
        {
            lastSequence = sequence;
        }
        if (!found || sequence > foundSequence)
        {
            memcpy(data, buffer, len);
            found = true;
            foundSequence = sequence;
            nextSlot = slot ^ 1;
        }
    }
    prefs.end();
    return found;
}

void SlotStore::erase()
{
    Preferences prefs;
    if (prefs.begin(nameSpace, false))
    {
        prefs.clear();
        prefs.end();
    }
    lastSequence = 0;
    nextSlot = 0;
}
