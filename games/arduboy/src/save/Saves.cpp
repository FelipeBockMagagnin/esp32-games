#include "Saves.h"
#include <Preferences.h>
#include <stdio.h>
#include <string.h>
#include "SaveCodec.h"

namespace Saves
{
    // Static rather than on the loop task's stack; only one save or load runs at a time
    static uint8_t packed[MAX_PACKED];
    static uint8_t check[MAX_PACKED];

    static void keyFor(uint32_t id, char *key)
    {
        snprintf(key, 12, "g%08lx", (unsigned long)id);
    }

    bool load(const char *nameSpace, uint32_t id, uint8_t *ram, size_t length)
    {
        char key[12];
        keyFor(id, key);
        Preferences prefs;
        if (!prefs.begin(nameSpace, true))
        {
            return false;
        }
        size_t size = prefs.isKey(key) ? prefs.getBytes(key, packed, sizeof(packed)) : 0;
        prefs.end();
        if (size == 0)
        {
            return false;
        }
        // Unpacked into the RAM only once it is known to fill it exactly
        return unpackSave(packed, size, nullptr, length) && unpackSave(packed, size, ram, length);
    }

    Result store(const char *nameSpace, uint32_t id, const uint8_t *ram, size_t length)
    {
        size_t size = packSave(ram, length, packed, sizeof(packed));
        if (size == 0)
        {
            return Result::TOO_BIG;
        }
        char key[12];
        keyFor(id, key);
        Preferences prefs;
        if (!prefs.begin(nameSpace, false))
        {
            return Result::NO_ROOM;
        }
        // Unchanged since the last save: no flash wear
        bool same = prefs.isKey(key) && prefs.getBytesLength(key) == size && prefs.getBytes(key, check, sizeof(check)) == size &&
                    memcmp(check, packed, size) == 0;
        bool ok = same || prefs.putBytes(key, packed, size) == size;
        prefs.end();
        return ok ? Result::SAVED : Result::NO_ROOM;
    }

    bool exists(const char *nameSpace, uint32_t id)
    {
        char key[12];
        keyFor(id, key);
        Preferences prefs;
        if (!prefs.begin(nameSpace, true))
        {
            return false;
        }
        bool found = prefs.isKey(key);
        prefs.end();
        return found;
    }
}
