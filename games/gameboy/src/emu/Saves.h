#ifndef SAVES_H
#define SAVES_H

#include <stddef.h>
#include <stdint.h>

// Each game's battery RAM, packed (SaveCodec) under a key of its own in this firmware's NVS
// namespace. Every firmware on the board shares the 20 KB NVS partition, so a save packs to
// MAX_PACKED bytes at most.
namespace Saves
{
    static const size_t MAX_PACKED = 2048;

    enum class Result : uint8_t
    {
        SAVED,
        TOO_BIG, // Packs to more than MAX_PACKED
        NO_ROOM  // NVS refused the write, most likely full
    };

    // False when there is no save, or it does not unpack to exactly `length` bytes; `ram` is
    // left alone then
    bool load(const char *nameSpace, uint32_t id, uint8_t *ram, size_t length);
    Result store(const char *nameSpace, uint32_t id, const uint8_t *ram, size_t length);
    bool exists(const char *nameSpace, uint32_t id);
}

#endif // SAVES_H
