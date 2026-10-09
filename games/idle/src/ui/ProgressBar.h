#ifndef UI_PROGRESS_BAR_H
#define UI_PROGRESS_BAR_H

#include "UI.h"

class ProgressBar : public UI
{
public:
    int32_t current;
    int32_t maxVal;
    uint16_t fillColor;
    uint16_t bgColor;
    uint16_t borderColor;
    bool showBorder;

    // Optional text centered over the bar (e.g. "23/100"). Drawn fresh every redraw, on top
    // of the fill, so the label never gets left over stale pixels when the fill width changes.
    String label;
    uint16_t labelColor;

    ProgressBar()
        : UI(0, 0, 0, 0),
          current(0),
          maxVal(100),
          fillColor(Theme::XP_BAR),
          bgColor(Theme::XP_TRACK),
          borderColor(Theme::XP_BORDER),
          showBorder(true),
          label(""),
          labelColor(Theme::TEXT),
          trailColor(0),
          trailValue(0),
          trailHoldUntil(0),
          lastTrailStep(0),
          now(0)
    {
    }

    ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, int32_t current, int32_t maxVal, uint16_t fillColor, uint16_t bgColor = Theme::XP_TRACK, uint16_t borderColor = Theme::XP_BORDER)
        : UI(x, y, w, h),
          current(current),
          maxVal(maxVal),
          fillColor(fillColor),
          bgColor(bgColor),
          borderColor(borderColor),
          showBorder(true),
          label(""),
          labelColor(Theme::TEXT),
          trailColor(0),
          trailValue(0),
          trailHoldUntil(0),
          lastTrailStep(0),
          now(0)
    {
    }

    // A drop then leaves the lost part lit in `color` for a moment before it drains away,
    // so a hit reads as a chunk taken out. Needs update() every loop.
    void enableTrail(uint16_t color)
    {
        trailColor = color;
    }

    void setProgress(int32_t val, int32_t max)
    {
        if (current == val && maxVal == max)
        {
            return;
        }
        // A rise (regen, a heal) leaves a trail that is still draining alone
        if (trailColor == 0 || max != maxVal)
        {
            trailValue = val;
        }
        else if (val < current)
        {
            if (trailValue < current)
            {
                trailValue = current;
            }
            trailHoldUntil = now + TRAIL_HOLD_MS;
        }
        current = val;
        maxVal = max;
        markDirty();
    }

    void update(unsigned long time);

    void setFillColor(uint16_t color)
    {
        if (fillColor != color)
        {
            fillColor = color;
            markDirty();
        }
    }

    void setBorderColor(uint16_t color)
    {
        if (borderColor != color)
        {
            borderColor = color;
            markDirty();
        }
    }

    void setLabel(const String &newLabel)
    {
        if (label != newLabel)
        {
            label = newLabel;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    static const unsigned long TRAIL_HOLD_MS = 250;

    uint16_t trailColor; // 0 while the trail is off
    int32_t trailValue;
    unsigned long trailHoldUntil;
    unsigned long lastTrailStep;
    unsigned long now;
};

#endif // UI_PROGRESS_BAR_H
