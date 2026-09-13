#include "ProgressBar.h"

void ProgressBar::draw(TFT_eSPI &tft)
{
    if (!visible || w <= 0 || h <= 0)
    {
        return;
    }

    if (maxVal <= 0)
    {
        maxVal = 1;
    }

    int32_t clamped = current;
    if (clamped < 0)
    {
        clamped = 0;
    }
    if (clamped > maxVal)
    {
        clamped = maxVal;
    }

    int16_t innerW = w - 2;
    int16_t innerH = h - 2;
    if (innerW < 0) innerW = 0;
    if (innerH < 0) innerH = 0;

    int16_t barW = (innerW > 0) ? (int16_t)(((int64_t)clamped * innerW) / maxVal) : 0;

    // Draw background
    if (bgColor != 0 || !showBorder)
    {
        tft.fillRect(x + 1, y + 1, innerW, innerH, bgColor);
    }

    // Draw filled portion
    if (barW > 0)
    {
        tft.fillRect(x + 1, y + 1, barW, innerH, fillColor);
    }

    // Draw border
    if (showBorder)
    {
        tft.drawRect(x, y, w, h, borderColor);
    }
}
