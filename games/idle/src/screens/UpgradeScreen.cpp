#include "UpgradeScreen.h"
#include "../enum/OreEnum.h"

UpgradeScreen::UpgradeScreen()
    : titleText(80, 12, "Upgrade", 0xFFFF, 2),
      headerLeftIcon(4, 7, 3, 5, image_ButtonLeftSmall_bits, false, 0xFFFF),
      backText(10, 5, "Mining"),
      headerRightIcon(235, 7, 3, 5, image_ButtonRightSmall_bits, false, 0xFFFF),
      nextText(197, 5, "Craft"),
      cooperDisplay(19, 40, "Cooper", OreEnum::COOPER),
      silverDisplay(102, 40, "Silver", OreEnum::SILVER),
      goldDisplay(182, 40, "Gold", OreEnum::GOLD),
      upgradeRow(5, 56, "Pickaxe", "0.1 cooper/s")
{
    addElement(&titleText);

    addElement(&headerLeftIcon);
    addElement(&backText);
    addElement(&headerRightIcon);
    addElement(&nextText);
    addElement(&cooperDisplay);
    addElement(&silverDisplay);
    addElement(&goldDisplay);
    addElement(&upgradeRow);
}

void UpgradeScreen::update(unsigned long now)
{
    upgradeRow.setCooperPrice(1000);
    upgradeRow.setSilverPrice(10);
    upgradeRow.setGoldPrice(20);
}
