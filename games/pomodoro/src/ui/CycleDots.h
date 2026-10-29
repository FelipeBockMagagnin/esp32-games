#ifndef UI_CYCLE_DOTS_H
#define UI_CYCLE_DOTS_H

#include "UI.h"

// A row of dots, one per focus in the cycle, filled for those done. Positioned by its centre.
class CycleDots : public UI
{
public:
    static const int16_t RADIUS = 5;
    static const int16_t PITCH = 20;

    CycleDots(int16_t cx, int16_t cy, uint8_t count)
        : UI(cx - (count * PITCH) / 2, cy - RADIUS, count * PITCH, 2 * RADIUS + 1),
          count(count), filled(0), color(Theme::BAD)
    {
    }

    void setFilled(uint8_t n)
    {
        if (filled != n)
        {
            filled = n;
            markDirty();
        }
    }

    void setColor(Color c)
    {
        if (color != c)
        {
            color = c;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override
    {
        for (uint8_t i = 0; i < count; i++)
        {
            int16_t cx = x + i * PITCH + PITCH / 2;
            int16_t cy = y + RADIUS;
            if (i < filled)
            {
                tft.fillCircle(cx, cy, RADIUS, color);
            }
            else
            {
                tft.fillCircle(cx, cy, RADIUS, eraseColor);
                tft.drawCircle(cx, cy, RADIUS, Theme::FRAME);
            }
        }
    }

private:
    uint8_t count;
    uint8_t filled;
    Color color;
};

#endif // UI_CYCLE_DOTS_H
