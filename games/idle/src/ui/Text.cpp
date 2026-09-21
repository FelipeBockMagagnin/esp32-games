#include "Text.h"

static int16_t getTextOriginX(int16_t x, int16_t w, uint8_t datum)
{
    switch (datum)
    {
    case TC_DATUM:
    case MC_DATUM:
    case BC_DATUM:
    case C_BASELINE:
        return x - (w / 2);
    case TR_DATUM:
    case MR_DATUM:
    case BR_DATUM:
    case R_BASELINE:
        return x - w;
    default:
        return x;
    }
}

static int16_t getTextOriginY(int16_t y, int16_t h, uint8_t datum)
{
    switch (datum)
    {
    case ML_DATUM:
    case MC_DATUM:
    case MR_DATUM:
        return y - (h / 2);
    case BL_DATUM:
    case BC_DATUM:
    case BR_DATUM:
        return y - h;
    case L_BASELINE:
    case C_BASELINE:
    case R_BASELINE:
        return y - (h * 8 / 10);
    default:
        return y;
    }
}

void Text::erase(TFT_eSPI &tft)
{
    if (prevW > 0 && prevH > 0)
    {
        int16_t clearX = getTextOriginX(drawnX, prevW, prevDatum);
        int16_t clearY = getTextOriginY(drawnY, prevH, prevDatum);
        tft.fillRect(clearX, clearY, prevW, prevH, bgColor);
    }
    prevW = 0;
    prevH = 0;
    prevText = "";
}

void Text::draw(TFT_eSPI &tft)
{
    if (text.length() == 0)
    {
        erase(tft);
        w = 0;
        h = 0;
        return;
    }

    // A datum change moves the text origin, so the old area must be cleared first
    if (prevW > 0 && prevDatum != datum)
    {
        erase(tft);
    }

    tft.setTextDatum(datum);
    tft.setTextSize(size);

    int16_t curW = tft.textWidth(text);
    int16_t curH = tft.fontHeight();

    w = curW;
    h = curH;

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
            int16_t clearX = getTextOriginX(x, clearW, datum);
            int16_t clearY = getTextOriginY(y, clearH, datum);
            tft.fillRect(clearX, clearY, clearW, clearH, bgColor);
        }
        tft.setTextColor(color);
        tft.drawString(text, x, y);
    }

    prevW = curW;
    prevH = curH;
    prevDatum = datum;
    prevText = text;
}
