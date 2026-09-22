#include "CoinDisplay.h"

void CoinDisplay::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    int16_t cx = x + DOT_RADIUS;
    int16_t cy = y + DOT_RADIUS;
    tft.drawEllipse(cx, cy, DOT_RADIUS, DOT_RADIUS, GOLD_COLOR);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFF);
    tft.drawString(text, cx + 11, cy - 3);
}
