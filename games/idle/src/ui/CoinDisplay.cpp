#include "CoinDisplay.h"

void CoinDisplay::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    // Dot stays a small accent and is vertically centered against however tall the text is
    int16_t cx = x + DOT_RADIUS;
    int16_t cy = y + h / 2;
    tft.drawEllipse(cx, cy, DOT_RADIUS, DOT_RADIUS, GOLD_COLOR);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(size);
    tft.setTextColor(0xFFFF);
    tft.drawString(text, cx + 11, y);
}
