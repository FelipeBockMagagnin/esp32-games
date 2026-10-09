#ifndef UI_COOLDOWN_RING_H
#define UI_COOLDOWN_RING_H

#include "UI.h"
#include "ButtonBadge.h"

// A ring that fills clockwise from 12 o'clock as a cooldown recovers, optionally around a
// drawing of the physical button that fires it. A full ring with a gold outline means
// "press now", and the ring flashes white the moment it gets one. Positioned by its centre; the area inside the ring (between the cap and the
// arc) is never painted, so another element (an icon) can sit there.
class CooldownRing : public UI
{
public:
    // The arc only redraws when it crosses one of these steps, which keeps a 10 Hz refresh
    // from repainting it for a change too small to see
    static const uint8_t STEPS = 36;
    static const int16_t THICKNESS = 3;
    static const int16_t CAP_RADIUS = 6;

    // Without a cap the centre is left to whatever is drawn there
    CooldownRing(int16_t cx, int16_t cy, int16_t radius)
        : CooldownRing(cx, cy, radius, ButtonBadge::CONFIRM, false)
    {
    }

    CooldownRing(int16_t cx, int16_t cy, int16_t radius, ButtonBadge::Kind kind, bool hasCap = true)
        : UI(cx - radius - 1, cy - radius - 1, 2 * radius + 3, 2 * radius + 3),
          radius(radius), kind(kind), hasCap(hasCap), step(0), ringColor(Theme::TEXT_DIM),
          trackColor(Theme::FRAME_DIM), outlined(false), outlineColor(Theme::SELECT), capDimmed(false),
          pingPending(false), pinging(false), pingStart(0)
    {
    }

    // progress in 0..1000, 1000 being ready
    void setProgress(uint16_t permille)
    {
        uint8_t s = (uint8_t)((uint32_t)(permille > 1000 ? 1000 : permille) * STEPS / 1000);
        if (step != s)
        {
            step = s;
            markDirty();
        }
    }

    void setRingColor(uint16_t color)
    {
        if (ringColor != color)
        {
            ringColor = color;
            markDirty();
        }
    }

    void setOutline(bool on)
    {
        if (outlined != on)
        {
            outlined = on;
            pingPending = on;
            pinging = false;
            markDirty();
        }
    }

    void setCapDimmed(bool dimmed)
    {
        if (capDimmed != dimmed)
        {
            capDimmed = dimmed;
            markDirty();
        }
    }

    // Times the ready flash; call after the setters
    void update(unsigned long now);

    void draw(TFT_eSPI &tft) override;

private:
    int16_t radius;
    ButtonBadge::Kind kind;
    bool hasCap;
    uint8_t step;
    uint16_t ringColor;
    uint16_t trackColor;
    bool outlined;
    uint16_t outlineColor;
    bool capDimmed;
    bool pingPending;
    bool pinging;
    unsigned long pingStart;
};

#endif // UI_COOLDOWN_RING_H
