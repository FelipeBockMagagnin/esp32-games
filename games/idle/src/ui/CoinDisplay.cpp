#include "CoinDisplay.h"
#include "../enum/OreEnum.h"

void CoinDisplay::draw(TFT_eSPI &tft)
{
    int color = 0;

    switch (oreType)
    {
    case OreEnum::COOPER:
        color = 0xE300;
        break;
    case OreEnum::SILVER:
        color = 0x73AF;
        break;
    case OreEnum::GOLD:
        color = 0xF685;
        break;
    default:
        break;
    }
    tft.drawEllipse(x, y, 3, 3, color);
    tft.setTextSize(1);
    tft.drawString("Cooper", x + 11, y - 3);
}
