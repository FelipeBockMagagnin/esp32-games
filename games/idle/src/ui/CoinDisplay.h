#ifndef UI_COIN_DISPLAY_H
#define UI_COIN_DISPLAY_H

#include "UI.h"
#include "../enum/OreEnum.h"

class CoinDisplay : public UI
{
public:
    static const int16_t DOT_RADIUS = 3;

    String text;
    OreEnum oreType;

    // (x, y) is the center of the ore dot; the element bounds start at its top-left corner
    CoinDisplay(int16_t x = 0, int16_t y = 0, const String &text = "", OreEnum oreType = OreEnum::COPPER)
        : UI(x - DOT_RADIUS, y - DOT_RADIUS, 74, 8),
          text(text),
          oreType(oreType)
    {
    }

    void setText(const String &newText)
    {
        if (text != newText)
        {
            text = newText;
            markDirty();
        }
    }

    void setOreType(OreEnum newOreType)
    {
        if (oreType != newOreType)
        {
            oreType = newOreType;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_COIN_DISPLAY_H
