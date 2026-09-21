#include "Ellipse.h"

void Ellipse::draw(TFT_eSPI &tft)
{
    if (!visible || rx <= 0 || ry <= 0)
    {
        return;
    }

    if (filled)
    {
        tft.fillEllipse(centerX, centerY, rx, ry, color);
    }
    else
    {
        tft.drawEllipse(centerX, centerY, rx, ry, color);
    }
}
