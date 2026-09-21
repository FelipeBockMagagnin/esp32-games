#include "Image.h"

void Image::draw(TFT_eSPI &tft)
{
    if (type == IMAGE_RGB565)
    {
        if (pixels == nullptr || w <= 0 || h <= 0)
        {
            return;
        }

        if (useTransparent)
        {
            tft.pushImage(x, y, w, h, (uint16_t *)pixels, color);
        }
        else
        {
            tft.pushImage(x, y, w, h, (uint16_t *)pixels);
        }
    }
    else if (type == IMAGE_BITMAP_1BIT)
    {
        if (bitmap == nullptr || w <= 0 || h <= 0)
        {
            return;
        }

        if (useTransparent)
        {
            tft.drawBitmap(x, y, bitmap, w, h, color);
        }
        else
        {
            tft.drawBitmap(x, y, bitmap, w, h, color, bgColor);
        }
    }
}