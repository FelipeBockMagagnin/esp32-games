#ifndef UI_BUTTON_BADGE_H
#define UI_BUTTON_BADGE_H

#include "UI.h"

// A small drawing of one of the physical buttons, so an on-screen action can point at the
// button that triggers it: Confirm is the red cap, Up and Down are white caps with an arrow.
// Positioned by its top-left corner like every other element; it is SIZE pixels square.
class ButtonBadge : public UI
{
public:
    enum Kind : uint8_t
    {
        CONFIRM,
        UP,
        DOWN
    };

    static const int16_t SIZE = 11;

    Kind kind;
    bool dimmed; // Drawn in dull tones while the action it labels is unavailable

    ButtonBadge(int16_t x, int16_t y, Kind kind)
        : UI(x, y, SIZE, SIZE),
          kind(kind),
          dimmed(false)
    {
    }

    void setDimmed(bool d)
    {
        if (dimmed != d)
        {
            dimmed = d;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_BUTTON_BADGE_H
