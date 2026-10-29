#ifndef UI_ICON_UTIL_H
#define UI_ICON_UTIL_H

#include <TFT_eSPI.h>

// Helpers for the 16x16 RGB565 icons in Assets.cpp. Their words are byte-swapped (pushImage
// sends them raw) and pure black is the transparent key.

// Largest icon the gray variant composes on the stack; bigger ones draw in color
static const int16_t ICON_GRAY_MAX_PIXELS = 16 * 16;

inline void pushIcon(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels)
{
    tft.pushImage(x, y, w, h, pixels, (uint16_t)0x0000);
}

// Draws the icon in gray at `brightnessPercent` of its own luminance, for something not
// owned or not reachable yet: the shape still reads, the color says "not yours"
inline void pushIconGray(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels,
                         uint8_t brightnessPercent)
{
    if (w * h > ICON_GRAY_MAX_PIXELS)
    {
        pushIcon(tft, x, y, w, h, pixels);
        return;
    }

    uint16_t buffer[ICON_GRAY_MAX_PIXELS];
    int16_t count = w * h;
    for (int16_t i = 0; i < count; i++)
    {
        uint16_t raw = pgm_read_word(pixels + i);
        if (raw == 0)
        {
            buffer[i] = 0;
            continue;
        }
        uint16_t color = (uint16_t)((raw >> 8) | (raw << 8));
        // Luminance from the 5/6/5 channels, scaled to 0..63
        uint16_t luma = (((color >> 11) * 2) * 77 + ((color >> 5) & 0x3F) * 150 + ((color & 0x1F) * 2) * 29) >> 8;
        luma = luma * brightnessPercent / 100;
        uint16_t gray = (uint16_t)(((luma >> 1) << 11) | (luma << 5) | (luma >> 1));
        if (gray == 0)
        {
            gray = 0x0020; // Stay opaque: pure black is the transparent key
        }
        buffer[i] = (uint16_t)((gray >> 8) | (gray << 8));
    }
    tft.pushImage(x, y, w, h, buffer, (uint16_t)0x0000);
}

#endif // UI_ICON_UTIL_H
