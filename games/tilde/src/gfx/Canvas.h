#ifndef GFX_CANVAS_H
#define GFX_CANVAS_H

#include <Arduino.h>
#include "Font.h"

// Draws into one band of the screen at a time: a buffer of 240 x `rows` RGB565 pixels, in
// screen coordinates, clipped to the band. Every shape is anti-aliased: each pixel's cover comes from
// the shape's signed distance and is alpha-blended in. Plain C++, so the PC tools draw the
// same pixels as the board.
class Canvas
{
public:
    static const int16_t W = 240;
    static const int16_t H = 320;
    static const int16_t BAND_H = 32; // the tools' default band

    enum Align : uint8_t
    {
        LEFT,
        CENTER,
        RIGHT
    };

    void begin(uint16_t *buffer, int16_t bandTop, int16_t bandRows = BAND_H);
    int16_t bandTop() const { return top; }
    int16_t bandRows() const { return rows; }
    // Whether rows y0..y1 reach this band, to skip whole groups of shapes
    bool reaches(float y0, float y1) const { return y1 >= top && y0 < top + rows; }

    // Nothing outside this box is drawn until clearClip()
    void setClip(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
    {
        clipX0 = x0;
        clipY0 = y0;
        clipX1 = x1;
        clipY1 = y1;
    }
    void clearClip() { setClip(0, -32768, W, 32767); }

    // Multiplies every alpha after it, for fading a whole group
    void setOpacity(uint8_t value) { opacity = value; }
    uint8_t getOpacity() const { return opacity; }

    void clear(uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t alpha = 255);
    void gradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t topColor, uint16_t bottomColor);
    void fillCircle(float cx, float cy, float r, uint16_t color, uint8_t alpha = 255);
    void fillEllipse(float cx, float cy, float rx, float ry, uint16_t color, uint8_t alpha = 255);
    void fillRoundRect(float x, float y, float w, float h, float r, uint16_t color, uint8_t alpha = 255);
    // From `topColor` at its top to `bottomColor` at its bottom
    void fillRoundRectV(float x, float y, float w, float h, float r, uint16_t topColor, uint16_t bottomColor,
                        uint8_t alpha = 255);
    void strokeRoundRect(float x, float y, float w, float h, float r, float width, uint16_t color,
                         uint8_t alpha = 255);
    // A thick line with round ends
    void line(float x0, float y0, float x1, float y1, float width, uint16_t color, uint8_t alpha = 255);
    void ring(float cx, float cy, float r, float width, uint16_t color, uint8_t alpha = 255);
    // Round-ended, from `start` to `end` radians, clockwise on screen (0 points right)
    void arc(float cx, float cy, float r, float width, float start, float end, uint16_t color, uint8_t alpha = 255);
    void fillTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color, uint8_t alpha = 255);

    // `y` is the top of the line; returns the width drawn
    int16_t text(float x, float y, const char *text, const Font &font, uint16_t color, Align align = LEFT,
                 uint8_t alpha = 255);
    static int16_t textWidth(const char *text, const Font &font);

    // `t` 0 is a, 255 is b
    static uint16_t mix(uint16_t a, uint16_t b, uint8_t t);

private:
    uint16_t *buf = nullptr;
    int16_t top = 0;
    int16_t rows = BAND_H;
    uint8_t opacity = 255;
    int16_t clipX0 = 0, clipY0 = -32768, clipX1 = W, clipY1 = 32767;

    void blend(int16_t x, int16_t y, uint16_t color, uint16_t alpha);
    // Calls `distance(px, py)` for every pixel centre in the box and blends by its cover
    template <typename F>
    void shade(float x0, float y0, float x1, float y1, uint16_t color, uint8_t alpha, F distance);
    // A convex shape row by row: `extent(py, left, right)` gives its span at a row's centre (false
    // when the row misses it); the inside is filled flat and only the pixels near the edge pay
    // for `distance`. `x0`..`x1` bounds the shape, for its first and last rows.
    template <typename Extent, typename Distance>
    void spans(float x0, float y0, float x1, float y1, uint16_t color, uint8_t alpha, Extent extent, Distance distance,
               uint16_t bottomColor = 0, float gradH = 0);
};

#endif // GFX_CANVAS_H
