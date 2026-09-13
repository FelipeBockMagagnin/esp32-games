#ifndef UI_BOX_H
#define UI_BOX_H

#include "UI.h"

class Box : public UI
{
public:
    uint16_t borderColor;
    uint16_t fillColor;
    bool filled;
    bool showBorder;
    bool rounded;
    int16_t radius;

    Box()
        : UI(0, 0, 0, 0),
          borderColor(0xFFFF),
          fillColor(0x0000),
          filled(false),
          showBorder(true),
          rounded(false),
          radius(0)
    {
    }

    Box(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t borderColor = 0xFFFF, bool filled = false, uint16_t fillColor = 0x0000, bool rounded = false, int16_t radius = 4)
        : UI(x, y, w, h),
          borderColor(borderColor),
          fillColor(fillColor),
          filled(filled),
          showBorder(true),
          rounded(rounded),
          radius(radius)
    {
    }

    void setBorderColor(uint16_t color)
    {
        if (borderColor != color)
        {
            borderColor = color;
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

    void setFilled(bool isFilled)
    {
        if (filled != isFilled)
        {
            filled = isFilled;
            markDirty();
        }
    }

    void setShowBorder(bool show)
    {
        if (showBorder != show)
        {
            showBorder = show;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_BOX_H
