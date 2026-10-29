#include "ArtImage.h"

void ArtImage::draw(TFT_eSPI &tft)
{
    // A row at a time, the transparent texels in the background's colour, byte-swapped like the art
    static uint16_t line[255 * 4];
    uint16_t bg = eraseColor;
    bg = (uint16_t)(bg >> 8 | bg << 8);
    const Bitmap &b = *bitmap;
    for (int16_t ty = 0; ty < b.h; ty++)
    {
        for (int16_t tx = 0; tx < b.w; tx++)
        {
            uint8_t index = b.texels[tx * b.h + ty];
            uint16_t color = index != 0 ? b.shades[index] : bg;
            for (uint8_t s = 0; s < scale; s++)
            {
                line[tx * scale + s] = color;
            }
        }
        for (uint8_t s = 0; s < scale; s++)
        {
            tft.pushImage(x, y + ty * scale + s, w, 1, line);
        }
    }
}
