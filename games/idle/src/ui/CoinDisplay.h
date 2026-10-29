#ifndef UI_COIN_DISPLAY_H
#define UI_COIN_DISPLAY_H

#include "UI.h"
#include "../game/GameConfig.h"

class CoinDisplay : public UI
{
public:
    static const int16_t DOT_RADIUS = 3;

    String text;
    uint8_t size;

    // (x, y) is the center of the gold dot; the element bounds start at its top-left corner.
    // Width is generous and fixed so it fits the amount at any size without per-size tuning.
    CoinDisplay(int16_t x = 0, int16_t y = 0, const String &text = "", uint8_t size = 1)
        : UI(x - DOT_RADIUS, y - DOT_RADIUS, 90, size * 8),
          text(text),
          size(size)
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

    void setSize(uint8_t newSize)
    {
        if (size != newSize)
        {
            size = newSize;
            h = newSize * 8;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_COIN_DISPLAY_H
