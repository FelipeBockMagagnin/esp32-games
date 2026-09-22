#ifndef UI_COIN_DISPLAY_H
#define UI_COIN_DISPLAY_H

#include "UI.h"
#include "../game/GameConfig.h"

class CoinDisplay : public UI
{
public:
    static const int16_t DOT_RADIUS = 3;

    String text;

    // (x, y) is the center of the gold dot; the element bounds start at its top-left corner
    CoinDisplay(int16_t x = 0, int16_t y = 0, const String &text = "")
        : UI(x - DOT_RADIUS, y - DOT_RADIUS, 74, 8),
          text(text)
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

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_COIN_DISPLAY_H
