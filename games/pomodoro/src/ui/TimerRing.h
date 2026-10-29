#ifndef UI_TIMER_RING_H
#define UI_TIMER_RING_H

#include "UI.h"

// A thick ring that empties clockwise from 12 o'clock as the time runs out. Positioned by its
// centre; the inside is never painted, so the time and phase can sit there.
class TimerRing : public UI
{
public:
    // The arc only redraws when it crosses one of these steps: at 25 minutes, every 12.5 s
    static const uint8_t STEPS = 120;

    TimerRing(int16_t cx, int16_t cy, int16_t radius, int16_t thickness)
        : UI(cx - radius, cy - radius, 2 * radius + 1, 2 * radius + 1),
          radius(radius), thickness(thickness), step(STEPS), ringColor(Theme::BAD), trackColor(Theme::FRAME_DIM)
    {
    }

    // permille of the phase still left
    void setRemaining(uint16_t permille)
    {
        uint8_t s = (uint8_t)(((uint32_t)(permille > 1000 ? 1000 : permille) * STEPS + 999) / 1000);
        if (step != s)
        {
            step = s;
            markDirty();
        }
    }

    void setRingColor(Color color)
    {
        if (ringColor != color)
        {
            ringColor = color;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    int16_t radius;
    int16_t thickness;
    uint8_t step;
    Color ringColor;
    Color trackColor;
};

#endif // UI_TIMER_RING_H
