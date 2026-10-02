#ifndef UI_UPGRADE_SLOT_H
#define UI_UPGRADE_SLOT_H

#include "UI.h"

// One cell of the upgrade grid: a rounded tile tinted with its category's accent color
// and a 16x16 icon centred on it. Frame, tint and icon are one element so a state change
// repaints them together; a Box under an Image would wipe the icon on every border change.
class UpgradeSlot : public UI
{
public:
    enum State : uint8_t
    {
        AFFORDABLE, // Accent frame and tint, icon in full color
        TOO_DEAR,   // Gray frame, icon faded toward the background
        SELECTED    // Thick yellow frame over the accent tint
    };

    static const int16_t ICON_SIZE = 16;

    UpgradeSlot(int16_t x = 0, int16_t y = 0, int16_t w = 34, int16_t h = 34)
        : UI(x, y, w, h),
          icon(nullptr),
          accent(0xFFFF),
          state(AFFORDABLE)
    {
    }

    // The icon is 16x16 RGB565 with black transparent, and must outlive the slot
    void setIcon(const uint16_t *pixels)
    {
        if (icon != pixels)
        {
            icon = pixels;
            markDirty();
        }
    }

    void setAccent(uint16_t color)
    {
        if (accent != color)
        {
            accent = color;
            markDirty();
        }
    }

    void setState(State s)
    {
        if (state != s)
        {
            state = s;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    const uint16_t *icon;
    uint16_t accent;
    State state;
};

#endif // UI_UPGRADE_SLOT_H
