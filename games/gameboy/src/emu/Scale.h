#ifndef SCALE_H
#define SCALE_H

#include <stdint.h>

// The Game Boy's 160x144 drawn at 1.5x, 240x216: the full width of the portrait panel.
// Each pair of source pixels becomes three (the first doubled), and each pair of lines too.
namespace Scale
{
    static const int SRC_W = 160;
    static const int SRC_H = 144;
    static const int OUT_W = 240;
    static const int OUT_H = 216;

    // pixels holds Peanut-GB's shade in its low two bits
    inline void line(const uint8_t *pixels, const uint16_t shades[4], uint16_t *out)
    {
        for (int x = 0; x < SRC_W; x += 2)
        {
            uint16_t a = shades[pixels[x] & 3];
            *out++ = a;
            *out++ = a;
            *out++ = shades[pixels[x + 1] & 3];
        }
    }

    // The output rows a source line fills: the even line two, the odd one one
    inline int firstRow(int line) { return line / 2 * 3 + (line & 1) * 2; }
    inline int rowCount(int line) { return (line & 1) ? 1 : 2; }
}

#endif // SCALE_H
