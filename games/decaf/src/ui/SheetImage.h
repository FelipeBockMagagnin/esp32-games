#ifndef UI_SHEET_IMAGE_H
#define UI_SHEET_IMAGE_H

#include "UI.h"
#include "../assets/Sprites.h"

// A frame of a sprite sheet, scaled up whole pixels, over a flat background
class SheetImage : public UI
{
public:
    SheetImage(int16_t x, int16_t y, const SheetDef &sheet, uint8_t scale, Color background);

    void setFrame(uint8_t index)
    {
        if (frame != index && index < sheet.count)
        {
            frame = index;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    static const int16_t MAX_W = 256;

    const SheetDef &sheet;
    uint8_t scale;
    Color background;
    uint8_t frame;
};

#endif // UI_SHEET_IMAGE_H
