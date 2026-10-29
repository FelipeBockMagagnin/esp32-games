#ifndef UI_WHEEL_VIEW_H
#define UI_WHEEL_VIEW_H

#include "UI.h"
#include "../dice/Spinner.h"

// The wheel under its pointer, painted off screen and pushed whole so a turning wheel never
// flickers. The sprite (62 KB) lives only while the screen is shown: release() frees it.
class WheelView : public UI
{
public:
    static const int16_t SIZE = 176;

    WheelView(TFT_eSPI &tft, const Spinner &spinner, int16_t x, int16_t y);

    // With a winner set, the other slices dim
    void setWinner(int8_t slice)
    {
        if (winner != slice)
        {
            winner = slice;
            markDirty();
        }
    }
    void release();

    // The colour slice i is painted in, for a label that matches it
    static Color sliceColor(uint8_t slice, uint8_t slices);

    void draw(TFT_eSPI &tft) override;

private:
    TFT_eSprite canvas;
    const Spinner &spinner;
    int8_t winner = -1;
    bool allocated = false;

    void paint(TFT_eSPI &g, int16_t ox, int16_t oy);
};

#endif // UI_WHEEL_VIEW_H
