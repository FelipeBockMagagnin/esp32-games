#include "Text.h"

void Text::draw(TFT_eSPI &tft)
{
    tft.setTextDatum(datum);
    tft.setTextSize(size);

    int16_t curW = tft.textWidth(text);
    int16_t curH = tft.fontHeight();

    if (!visible || text.length() == 0)
    {
        if (prevW > 0 && prevH > 0)
        {
            tft.fillRect(x, y, prevW, prevH, bgColor);
            prevW = 0;
            prevH = 0;
            prevText = "";
        }
        return;
    }

    if (!transparentBg)
    {
        if (prevW > curW)
        {
            tft.setTextPadding(prevW);
        }
        tft.setTextColor(color, bgColor);
        tft.drawString(text, x, y);
        tft.setTextPadding(0);
    }
    else
    {
        if (prevW > 0 && text != prevText)
        {
            int16_t clearW = (prevW > curW) ? prevW : curW;
            int16_t clearH = (prevH > curH) ? prevH : curH;
            tft.fillRect(x, y, clearW, clearH, bgColor);
        }
        tft.setTextColor(color);
        tft.drawString(text, x, y);
    }

    prevW = curW;
    prevH = curH;
    prevText = text;
}
