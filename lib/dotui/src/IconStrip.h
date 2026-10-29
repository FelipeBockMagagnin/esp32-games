#ifndef UI_ICON_STRIP_H
#define UI_ICON_STRIP_H

#include "UI.h"

// A row of 16x16 icons, each in color or grayed out, drawn as one element so the
// strip can change length without its icons erasing one another
class IconStrip : public UI
{
public:
    static const uint8_t MAX_ICONS = 10;
    static const int16_t ICON_SIZE = 16;
    static const int16_t PITCH = 20;

    IconStrip(int16_t x = 0, int16_t y = 0)
        : UI(x, y, MAX_ICONS * PITCH, ICON_SIZE),
          count(0)
    {
        for (uint8_t i = 0; i < MAX_ICONS; i++)
        {
            icons[i] = nullptr;
            gray[i] = false;
        }
    }

    // Icons past the count are ignored; call setCount after filling them
    void setIcon(uint8_t index, const uint16_t *pixels, bool grayed)
    {
        if (index >= MAX_ICONS || (icons[index] == pixels && gray[index] == grayed))
        {
            return;
        }
        icons[index] = pixels;
        gray[index] = grayed;
        markDirty();
    }

    void setCount(uint8_t n)
    {
        if (n > MAX_ICONS)
        {
            n = MAX_ICONS;
        }
        if (count != n)
        {
            count = n;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    const uint16_t *icons[MAX_ICONS];
    bool gray[MAX_ICONS];
    uint8_t count;
};

#endif // UI_ICON_STRIP_H
