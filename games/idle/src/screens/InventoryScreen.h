#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"

class InventoryScreen : public Screen
{
private:
    Text titleText;
    // tft.drawString("Inventory", 70, 10);

    Image playerAttackIcon;
    Text playerAttackText;

    // tft.pushImage(16, 285, 16, 16, image_sword_02b_pixels);
    // tft.drawString("100", 33, 286);

    Image playerDefenceIcon;
    Text playerDefenceText;

    // tft.pushImage(99, 285, 16, 16, image_armor_01b_pixels);
    // tft.drawString("100", 116, 286);

    Image playerHeartIcon;
    Text playerHeartText;

    // tft.drawBitmap(174, 285, image_cards_hearts_bits, 15, 16, 0xC0E5);
    // tft.drawString("20", 192, 286);

    Box ringSlotBox;
    // tft.drawRect(57, 69, 32, 32, 0xFFFF);

    Box ringSlot2Box;
    // tft.drawRect(57, 114, 32, 32, 0xFFFF);

    Box helmetSlotBox;
    // tft.drawRect(106, 69, 32, 32, 0xFFFF);

    Box breastplateSlotBox;
    // tft.drawRect(106, 114, 32, 32, 0xFFFF);

    Box legSlotBox;
    // tft.drawRect(106, 161, 32, 32, 0xFFFF);

    Box bootSlotBox;
    // tft.drawRect(106, 206, 32, 32, 0xFFFF);

    Box amuletSlotBox;
    // tft.drawRect(156, 69, 32, 32, 0xFFFF);

    Box swordSlotBox;
    // tft.drawRect(156, 114, 32, 32, 0xFFFF);

public:
    InventoryScreen();

    void update(unsigned long now) override;
};

#endif // INVENTORY_SCREEN_H
