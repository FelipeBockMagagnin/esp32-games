#include "Ellipse.h"

void Ellipse::draw(TFT_eSPI &tft)
{
    if (rx <= 0 || ry <= 0)
    {
        return;
    }

    // Clear first so switching from filled to outline doesn't leave the old fill behind
    tft.fillRect(x, y, w, h, eraseColor);

    if (filled)
    {
        tft.fillEllipse(centerX(), centerY(), rx, ry, color);
    }
    else
    {
        tft.drawEllipse(centerX(), centerY(), rx, ry, color);
    }
}
