#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "UI.h"

enum ImageType
{
    IMAGE_RGB565,
    IMAGE_BITMAP_1BIT
};

class Image : public UI
{
public:
    ImageType type;
    const uint16_t *pixels;
    const uint8_t *bitmap;
    uint16_t color;        // Foreground color for 1-bit bitmap or transparent key for RGB565
    uint16_t bgColor;      // Background color for 1-bit bitmap
    bool useTransparent;   // True if RGB565 has transparent color key or 1-bit has transparent bg

    Image()
        : UI(0, 0, 0, 0),
          type(IMAGE_RGB565),
          pixels(nullptr),
          bitmap(nullptr),
          color(0xFFFF),
          bgColor(0x0000),
          useTransparent(false)
    {
    }

    // 16-bit RGB565 Image constructor
    Image(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, bool hasTransparent = false, uint16_t transColor = 0)
        : UI(x, y, w, h),
          type(IMAGE_RGB565),
          pixels(pixels),
          bitmap(nullptr),
          color(transColor),
          bgColor(0x0000),
          useTransparent(hasTransparent)
    {
    }

    // 1-bit Monochrome Bitmap constructor
    Image(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap, uint16_t color = 0xFFFF, bool transparentBg = true, uint16_t bgColor = 0x0000)
        : UI(x, y, w, h),
          type(IMAGE_BITMAP_1BIT),
          pixels(nullptr),
          bitmap(bitmap),
          color(color),
          bgColor(bgColor),
          useTransparent(transparentBg)
    {
    }

    void setPixels(const uint16_t *newPixels, int16_t newW, int16_t newH, bool hasTrans = false, uint16_t transColor = 0)
    {
        pixels = newPixels;
        w = newW;
        h = newH;
        type = IMAGE_RGB565;
        useTransparent = hasTrans;
        color = transColor;
        markDirty();
    }

    void setBitmap(const uint8_t *newBitmap, int16_t newW, int16_t newH, uint16_t newColor = 0xFFFF, bool transBg = true, uint16_t newBg = 0x0000)
    {
        bitmap = newBitmap;
        w = newW;
        h = newH;
        type = IMAGE_BITMAP_1BIT;
        color = newColor;
        bgColor = newBg;
        useTransparent = transBg;
        markDirty();
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_IMAGE_H
