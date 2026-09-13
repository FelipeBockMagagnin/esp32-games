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

    ProgressBar()
        : UI(0, 0, 0, 0),
          current(0),
          maxVal(100),
          fillColor(0x15D0),
          bgColor(0x0000),
          borderColor(0xFFFF),
          showBorder(true)
    {
    }

    ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, int32_t current, int32_t maxVal, uint16_t fillColor, uint16_t bgColor = 0x0000, uint16_t borderColor = 0xFFFF)
        : UI(x, y, w, h),
          current(current),
          maxVal(maxVal),
          fillColor(fillColor),
          bgColor(bgColor),
          borderColor(borderColor),
          showBorder(true)
    {
    }

    void setProgress(int32_t val, int32_t max)
    {
        if (current != val || maxVal != max)
        {
            current = val;
            maxVal = max;
            markDirty();
        }
    }

    void setFillColor(uint16_t color)
    {
        if (fillColor != color)
        {
            fillColor = color;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_PROGRESS_BAR_H
