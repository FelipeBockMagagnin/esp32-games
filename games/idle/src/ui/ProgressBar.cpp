#include "ProgressBar.h"

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

    // Draw filled portion
    if (barW > 0)
    {
        tft.fillRect(x + 1, y + 1, barW, innerH, fillColor);
    }

    // Clear remaining unfilled portion (erases old progress on decrease without full flicker)
    if (innerW > barW)
    {
        tft.fillRect(x + 1 + barW, y + 1, innerW - barW, innerH, bgColor);
    }

    // Draw border
    if (showBorder)
    {
        tft.drawRect(x, y, w, h, borderColor);
    }
}

