#include "IconStrip.h"
#include "IconUtil.h"

static const uint8_t GRAY_PERCENT = 50;

void IconStrip::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);
    for (uint8_t i = 0; i < count; i++)
    {
        if (icons[i] == nullptr)
        {
            continue;
        }
        int16_t iconX = x + i * PITCH;
        if (gray[i])
        {
            pushIconGray(tft, iconX, y, ICON_SIZE, ICON_SIZE, icons[i], GRAY_PERCENT);
        }
        else
        {
            pushIcon(tft, iconX, y, ICON_SIZE, ICON_SIZE, icons[i]);
        }
    }
}
