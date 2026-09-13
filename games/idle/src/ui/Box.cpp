#include "Box.h"

void Box::draw(TFT_eSPI &tft)
{
    if (!visible || w <= 0 || h <= 0)
    {
        return;
    }

    if (rounded)
    {
        if (filled)
        {
            tft.fillRoundRect(x, y, w, h, radius, fillColor);
        }
        if (showBorder)
        {
            tft.drawRoundRect(x, y, w, h, radius, borderColor);
        }
    }
    else
    {
        if (filled)
        {
            tft.fillRect(x, y, w, h, fillColor);
        }
        if (showBorder)
        {
            tft.drawRect(x, y, w, h, borderColor);
        }
    }
}
