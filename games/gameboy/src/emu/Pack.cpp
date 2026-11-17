#include "Pack.h"
#include <string.h>

// The pack sits in flash with no alignment promise for its fields, so they are read a byte at
// a time
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

bool Pack::open(const uint8_t *blob, size_t size)
{
    entries = 0;
    if (size < HEADER || memcmp(blob, "GBPK", 4) != 0 || le16(blob + 6) != VERSION)
    {
        return false;
    }
    uint16_t count = le16(blob + 4);
    if (HEADER + (size_t)count * ENTRY > size)
    {
        return false;
    }
    data = blob;
    length = size;
    for (uint16_t i = 0; i < count; i++)
    {
        const uint8_t *p = blob + HEADER + (size_t)i * ENTRY;
        uint32_t offset = le32(p), romSize = le32(p + 4);
        if (offset > size || romSize > size - offset || romSize < 0x150)
        {
            return false;
        }
    }
    entries = count;
    return true;
}

PackEntry Pack::entry(uint16_t index) const
{
    PackEntry e = {};
    if (index >= entries)
    {
        return e;
    }
    const uint8_t *p = data + HEADER + (size_t)index * ENTRY;
    e.offset = le32(p);
    e.size = le32(p + 4);
    e.id = le32(p + 8);
    e.okIsB = p[12] == 1;
    memcpy(e.title, p + 16, sizeof(e.title));
    memcpy(e.author, p + 40, sizeof(e.author));
    e.title[sizeof(e.title) - 1] = '\0';
    e.author[sizeof(e.author) - 1] = '\0';
    return e;
}
