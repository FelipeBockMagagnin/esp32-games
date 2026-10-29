#include "Chip.h"

void Chip::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);
    tft.fillRoundRect(x, y, w, h, 4, latest ? Theme::ROW_SELECTED : Theme::FRAME_DIM);
    tft.drawRoundRect(x, y, w, h, 4, latest ? Theme::SELECT : Theme::FRAME);

    char label[8];
    snprintf(label, sizeof(label), "%d", (int)value);
    tft.setTextFont(1);
    tft.setTextSize(strlen(label) > 3 ? 1 : 2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(latest ? Theme::TEXT : Theme::TEXT_2);
    tft.drawString(label, x + w / 2 + 1, y + h / 2 + 1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
}
