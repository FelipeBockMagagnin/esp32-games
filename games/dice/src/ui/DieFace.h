#ifndef UI_DIE_FACE_H
#define UI_DIE_FACE_H

#include "UI.h"

// One die on the table, in the silhouette of its kind (a d4's triangle, a d20's hexagon...)
// with its face value, or pips on a d6. Value 0 draws it blank, for the pool before a roll.
// Its square is cleared to eraseColor first, so register it on the card it sits on.
class DieFace : public UI
{
public:
    enum Style : uint8_t
    {
        KEPT,
        ASIDE, // Set aside by a keep: dimmed
        GOOD,
        BAD,
        ROLLING
    };

    static const int16_t SIZE = 36;

    DieFace() : UI(0, 0, SIZE, SIZE) {}

    void set(uint8_t newSides, uint8_t newValue, Style newStyle)
    {
        if (sides != newSides || value != newValue || style != newStyle)
        {
            sides = newSides;
            value = newValue;
            style = newStyle;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    uint8_t sides = 6;
    uint8_t value = 0;
    Style style = KEPT;

    void drawPips(TFT_eSPI &tft, uint16_t color);
};

#endif // UI_DIE_FACE_H
