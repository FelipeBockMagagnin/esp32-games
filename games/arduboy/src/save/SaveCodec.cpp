#include "SaveCodec.h"
#include <string.h>

static const size_t MAX_RUN = 128;

size_t packSave(const uint8_t *in, size_t length, uint8_t *out, size_t capacity)
{
    size_t i = 0, o = 0;
    while (i < length)
    {
        size_t run = 1;
        while (i + run < length && run < MAX_RUN && in[i + run] == in[i])
        {
            run++;
        }
        if (run >= 3)
        {
            if (o + 2 > capacity)
            {
                return 0;
            }
            out[o++] = (uint8_t)(257 - run);
            out[o++] = in[i];
            i += run;
            continue;
        }
        // Literals until the next run of three
        size_t literal = 0;
        while (i + literal < length && literal < MAX_RUN)
        {
            const uint8_t *p = in + i + literal;
            if (i + literal + 2 < length && p[0] == p[1] && p[1] == p[2])
            {
                break;
            }
            literal++;
        }
        if (o + 1 + literal > capacity)
        {
            return 0;
        }
        out[o++] = (uint8_t)(literal - 1);
        memcpy(out + o, in + i, literal);
        o += literal;
        i += literal;
    }
    return o;
}

bool unpackSave(const uint8_t *in, size_t packed, uint8_t *out, size_t length)
{
    size_t i = 0, o = 0;
    while (i < packed)
    {
        uint8_t n = in[i++];
        if (n < 128)
        {
            size_t count = (size_t)n + 1;
            if (i + count > packed || o + count > length)
            {
                return false;
            }
            if (out != nullptr)
            {
                memcpy(out + o, in + i, count);
            }
            i += count;
            o += count;
        }
        else if (n > 128)
        {
            size_t count = 257 - (size_t)n;
            if (i >= packed || o + count > length)
            {
                return false;
            }
            if (out != nullptr)
            {
                memset(out + o, in[i], count);
            }
            i++;
            o += count;
        }
    }
    return o == length;
}
