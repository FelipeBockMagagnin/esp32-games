#ifndef GFX_FONT_H
#define GFX_FONT_H

#include <Arduino.h>

// A glyph of printable ASCII: its pen advance, the offset of its bitmap from the pen at the
// line's top, and where its 4-bit alpha rows start in the font's bitmap
struct Glyph
{
    uint8_t advance;
    int8_t x;
    int8_t y;
    uint8_t w;
    uint8_t h;
    uint16_t offset;
};

struct Font
{
    uint8_t height;
    uint8_t ascent;
    const Glyph *glyphs; // ' ' to '~'
    const uint8_t *bits;
};

extern const Font FONT_SMALL;
extern const Font FONT_BODY;
extern const Font FONT_TITLE;

#endif // GFX_FONT_H
