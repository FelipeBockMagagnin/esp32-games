#include "TimerRing.h"

// TFT_eSPI measures arc angles clockwise from 6 o'clock, so 12 o'clock is 180
static const uint16_t TOP = 180;

void TimerRing::draw(TFT_eSPI &tft)
{
    int16_t cx = x + radius;
    int16_t cy = y + radius;
    int16_t inner = radius - thickness + 1;
    uint16_t sweep = (uint16_t)((uint32_t)step * 360 / STEPS);
    if (sweep >= 360)
    {
        tft.drawArc(cx, cy, radius, inner, 0, 360, ringColor, eraseColor, false);
    }
    else if (sweep == 0)
    {
        tft.drawArc(cx, cy, radius, inner, 0, 360, trackColor, eraseColor, false);
    }
    else
    {
        // What is left runs from the edge on to 12 o'clock; what has gone is the track before it
        uint16_t edge = (TOP + 360 - sweep) % 360;
        tft.drawArc(cx, cy, radius, inner, TOP, edge, trackColor, eraseColor, false);
        tft.drawArc(cx, cy, radius, inner, edge, TOP, ringColor, eraseColor, false);
    }
}
