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
        : UI(0, 0, 0, 0), rx(0), ry(0), color(0xFFFF), filled(false)
    {
    }

    Ellipse(int16_t centerX, int16_t centerY, int16_t rx, int16_t ry, uint16_t color = 0xFFFF, bool filled = false)
        : UI(centerX - rx, centerY - ry, rx * 2 + 1, ry * 2 + 1),
          rx(rx),
          ry(ry),
          color(color),
          filled(filled)
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

    // x/y hold the top-left corner, so the center is derived from them
    int16_t centerX() const { return x + rx; }
    int16_t centerY() const { return y + ry; }

    void setCenter(int16_t cx, int16_t cy)
    {
        setPosition(cx - rx, cy - ry);
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_ELLIPSE_H
