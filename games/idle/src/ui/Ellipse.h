#ifndef UI_ELLIPSE_H
#define UI_ELLIPSE_H

#include "UI.h"

class Ellipse : public UI
{
public:
    int16_t rx;
    int16_t ry;
    uint16_t color;
    bool filled;

    Ellipse()
        : UI(0, 0, 0, 0), rx(0), ry(0), color(0xFFFF), filled(false), centerX(0), centerY(0)
    {
    }

    Ellipse(int16_t centerX, int16_t centerY, int16_t rx, int16_t ry, uint16_t color = 0xFFFF, bool filled = false)
        : UI(centerX - rx, centerY - ry, rx * 2 + 1, ry * 2 + 1),
          rx(rx),
          ry(ry),
          color(color),
          filled(filled),
          centerX(centerX),
          centerY(centerY)
    {
    }

    void setColor(uint16_t c)
    {
        if (color != c)
        {
            color = c;
            markDirty();
        }
    }

    void setFilled(bool f)
    {
        if (filled != f)
        {
            filled = f;
            markDirty();
        }
    }

    void setCenter(int16_t cx, int16_t cy)
    {
        if (centerX != cx || centerY != cy)
        {
            centerX = cx;
            centerY = cy;
            x = cx - rx;
            y = cy - ry;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    int16_t centerX;
    int16_t centerY;
};

#endif // UI_ELLIPSE_H
