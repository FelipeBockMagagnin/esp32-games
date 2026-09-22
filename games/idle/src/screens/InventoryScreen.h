#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"

class InventoryScreen : public Screen
{
private:
    Header header;

    Image playerAttackIcon;
    Text playerAttackText;

    Image playerDefenceIcon;
    Text playerDefenceText;

    Image playerHeartIcon;
    Text playerHeartText;

    Box ringSlotBox;
    Box ringSlot2Box;
    Box helmetSlotBox;
    Box breastplateSlotBox;
    Box legSlotBox;
    Box bootSlotBox;
    Box amuletSlotBox;
    Box swordSlotBox;

public:
    InventoryScreen();

    void update(unsigned long now) override;
};

#endif // INVENTORY_SCREEN_H
