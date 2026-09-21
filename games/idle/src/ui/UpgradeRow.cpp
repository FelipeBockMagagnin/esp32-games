#include "UpgradeRow.h"
#include "../assets/Assets.h"
#include "../enum/OreEnum.h"

void UpgradeRow::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(0xFFFF);
    tft.setTextSize(2);
    tft.drawString(upgradeName, x + 39, y + 3);

    tft.drawRect(x, y, 32, 33, 0xFFFF);
    tft.drawBitmap(x + 11, y + 9, image_cursor_black_white_bits, 11, 16, 0xFFFF);

    // Description Square
    tft.setTextSize(1);
    tft.drawString(upgradeDescription, x + 41, y + 19);
    tft.drawRect(x + 31, y, 119, 33, 0xFFFF);

    // Price Square
    tft.drawRect(x + 149, y, 51, 33, 0xFFFF);

    tft.drawString(String(copperPrice), x + 163, y + 3);
    tft.drawString(String(silverPrice), x + 163, y + 13);
    tft.drawString(String(goldPrice), x + 163, y + 23);

    tft.drawEllipse(x + 156, y + 6, 3, 3, getOreInfo(OreEnum::COPPER).color);
    tft.drawEllipse(x + 156, y + 16, 3, 3, getOreInfo(OreEnum::SILVER).color);
    tft.drawEllipse(x + 156, y + 26, 3, 3, getOreInfo(OreEnum::GOLD).color);

    // Buy Square
    tft.drawRect(x + 199, y, 31, 33, 0xFFFF);
    tft.drawString(buyCount, x + 211, y + 2);
    tft.drawString("Buy", x + 206, y + 12);
}
