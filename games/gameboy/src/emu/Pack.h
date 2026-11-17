#ifndef PACK_H
#define PACK_H

#include <stddef.h>
#include <stdint.h>

// The ROMs built into the firmware, laid out by tools/pack.py (which documents the format;
// change both together)
struct PackEntry
{
    uint32_t offset; // From the start of the pack
    uint32_t size;
    uint32_t id;      // Names the game's save
    bool okIsB;       // The game's main action is on B, so OK presses B and Left+Right A
    char title[24];
    char author[24];
};

class Pack
{
public:
    static const uint16_t VERSION = 1;

    // False when the blob is not a pack of this version, or an entry points outside it
    bool open(const uint8_t *data, size_t length);

    uint16_t count() const { return entries; }
    PackEntry entry(uint16_t index) const;
    const uint8_t *rom(const PackEntry &e) const { return data + e.offset; }

private:
    static const size_t HEADER = 8;
    static const size_t ENTRY = 64;

    const uint8_t *data = nullptr;
    size_t length = 0;
    uint16_t entries = 0;
};

#endif // PACK_H
