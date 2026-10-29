#ifndef UI_ART_IMAGE_H
#define UI_ART_IMAGE_H

#include "UI.h"
#include "../render/Art.h"

// One of the view's bitmaps, fully lit, at a whole-number scale over the element's erase colour
class ArtImage : public UI
{
public:
    ArtImage(int16_t x, int16_t y, const Bitmap *bitmap, uint8_t scale)
        : UI(x, y, bitmap->w * scale, bitmap->h * scale), bitmap(bitmap), scale(scale) {}

    void setBitmap(const Bitmap *b)
    {
        if (b != bitmap)
        {
            bitmap = b;
            w = b->w * scale;
            h = b->h * scale;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    const Bitmap *bitmap;
    uint8_t scale;
};

#endif // UI_ART_IMAGE_H
