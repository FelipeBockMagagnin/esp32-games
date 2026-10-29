#include "ProgressBar.h"

static const unsigned long TRAIL_STEP_MS = 33;
static const uint8_t TRAIL_DRAIN_DIVISOR = 4; // Each step closes this fraction of the gap

void ProgressBar::update(unsigned long time)
{
    now = time;
    if (trailValue <= current || (long)(now - trailHoldUntil) < 0 || now - lastTrailStep < TRAIL_STEP_MS)
    {
        return;
    }
    lastTrailStep = now;
    int32_t step = (trailValue - current) / TRAIL_DRAIN_DIVISOR;
    trailValue -= step > 0 ? step : trailValue - current;
    markDirty();
}

void ProgressBar::draw(TFT_eSPI &tft)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }

    int32_t safeMax = (maxVal > 0) ? maxVal : 1;

    int32_t clamped = current;
    if (clamped < 0)
    {
        clamped = 0;
    }
    if (clamped > safeMax)
    {
        clamped = safeMax;
    }

    int16_t innerW = w - 2;
    int16_t innerH = h - 2;
    if (innerW < 0) innerW = 0;
    if (innerH < 0) innerH = 0;

    int16_t barW = (innerW > 0) ? (int16_t)(((int64_t)clamped * innerW) / safeMax) : 0;

    int32_t trail = trailValue < clamped ? clamped : (trailValue > safeMax ? safeMax : trailValue);
    int16_t trailW = (innerW > 0) ? (int16_t)(((int64_t)trail * innerW) / safeMax) : 0;

    if (barW > 0)
    {
        tft.fillRect(x + 1, y + 1, barW, innerH, fillColor);
    }
    if (trailW > barW)
    {
        tft.fillRect(x + 1 + barW, y + 1, trailW - barW, innerH, trailColor);
    }

    // Clear remaining unfilled portion (erases old progress on decrease without full flicker)
    if (innerW > trailW)
    {
        tft.fillRect(x + 1 + trailW, y + 1, innerW - trailW, innerH, bgColor);
    }

    // Draw border
    if (showBorder)
    {
        tft.drawRect(x, y, w, h, borderColor);
    }

    // Label last, on top of the fill; drawn every redraw so it never gets partially erased
    // by a fill/clear that only repaints part of the bar
    if (label.length() > 0)
    {
        tft.setTextColor(labelColor);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(label, x + w / 2, y + h / 2);
        tft.setTextDatum(TL_DATUM);
    }
}

