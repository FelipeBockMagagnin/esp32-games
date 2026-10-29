#include "SheetImage.h"

SheetImage::SheetImage(int16_t x, int16_t y, const SheetDef &sheet, uint8_t scale, Color background)
    : UI(x, y, sheet.frameW * scale, sheet.frameH * scale), sheet(sheet), scale(scale), background(background),
      frame(0)
{
}

void SheetImage::draw(TFT_eSPI &tft)
{
    static uint16_t line[MAX_W];
    uint16_t bg = (uint16_t)background;
    bg = (uint16_t)((bg >> 8) | (bg << 8));
    int16_t width = w < MAX_W ? w : MAX_W;
    const FrameDef &f = sheet.frames[frame];

    for (int16_t row = 0; row < sheet.frameH; row++)
    {
        bool inBox = f.pixels != nullptr && row >= f.y && row < f.y + f.h;
        for (int16_t col = 0; col < sheet.frameW; col++)
        {
            uint16_t c = bg;
            if (inBox && col >= f.x && col < f.x + f.w)
            {
                uint8_t index = pgm_read_byte(f.pixels + (row - f.y) * f.w + col - f.x);
                if (index != 0)
                {
                    c = pgm_read_word(sheet.palette + index - 1);
                }
            }
            for (uint8_t s = 0; s < scale && col * scale + s < width; s++)
            {
                line[col * scale + s] = c;
            }
        }
        for (uint8_t s = 0; s < scale; s++)
        {
            tft.pushImage(x, y + row * scale + s, width, 1, line);
        }
    }
}
