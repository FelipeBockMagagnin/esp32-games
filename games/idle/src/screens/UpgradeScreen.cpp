#include "UpgradeScreen.h"
#include "../enum/OreEnum.h"

UpgradeScreen::UpgradeScreen()
    : titleText(80, 12, "Upgrade", 0xFFFF, 2),
      headerLeftIcon(4, 7, 3, 5, image_ButtonLeftSmall_bits, 0xFFFF),
      backText(10, 5, "Mining"),
      headerRightIcon(235, 7, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),
      nextText(197, 5, "Craft"),
      copperDisplay(19, 40, getOreInfo(OreEnum::COPPER).name, OreEnum::COPPER),
      silverDisplay(102, 40, getOreInfo(OreEnum::SILVER).name, OreEnum::SILVER),
      goldDisplay(182, 40, getOreInfo(OreEnum::GOLD).name, OreEnum::GOLD),
      upgradeRow(5, 56, "Pickaxe", "0.1 copper/s")
{
    addElement(&titleText);

    addElement(&headerLeftIcon);
    addElement(&backText);
    addElement(&headerRightIcon);
    addElement(&nextText);
    addElement(&copperDisplay);
    addElement(&silverDisplay);
    addElement(&goldDisplay);
    addElement(&upgradeRow);

    upgradeRow.setCopperPrice(1000);
    upgradeRow.setSilverPrice(10);
    upgradeRow.setGoldPrice(20);
}

void UpgradeScreen::update(unsigned long now)
{
    // Dynamic updates bound to game state will be handled here
}

