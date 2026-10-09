#include "CooldownRing.h"

// TFT_eSPI measures arc angles clockwise from 6 o'clock, so 12 o'clock is 180
static const uint16_t TOP = 180;

static const unsigned long PING_MS = 200;
static const uint16_t PING_COLOR = Theme::TEXT;

void CooldownRing::update(unsigned long now)
{
    if (pingPending)
    {
        pingPending = false;
        pinging = true;
        pingStart = now;
        markDirty();
    }
    else if (pinging && now - pingStart >= PING_MS)
    {
        pinging = false;
        markDirty();
    }
}

void CooldownRing::draw(TFT_eSPI &tft)
{
    int16_t cx = x + radius + 1;
    int16_t cy = y + radius + 1;
    int16_t inner = radius - THICKNESS + 1;

    // The outline sits one pixel outside the arc and is painted out when not ready
    tft.drawCircle(cx, cy, radius + 1, pinging ? PING_COLOR : outlined ? outlineColor : eraseColor);

    uint16_t arcColor = pinging ? PING_COLOR : ringColor;
    uint16_t sweep = (uint16_t)step * 360 / STEPS;
    if (sweep >= 360)
    {
        tft.drawArc(cx, cy, radius, inner, 0, 360, arcColor, eraseColor, false);
    }
    else if (sweep == 0)
    {
        tft.drawArc(cx, cy, radius, inner, 0, 360, trackColor, eraseColor, false);
    }
    else
    {
        uint16_t edge = (TOP + sweep) % 360;
        tft.drawArc(cx, cy, radius, inner, TOP, edge, arcColor, eraseColor, false);
        tft.drawArc(cx, cy, radius, inner, edge, TOP, trackColor, eraseColor, false);
    }

    if (hasCap)
    {
        ButtonBadge::drawCap(tft, cx, cy, CAP_RADIUS, kind, capDimmed);
    }
}
