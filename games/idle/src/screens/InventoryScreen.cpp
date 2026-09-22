#include "InventoryScreen.h"

InventoryScreen::InventoryScreen()
    : Screen(0x0000),
      header("Inventory", "Upgrade", "Combat"),

      playerAttackIcon(16, 285, 16, 16, image_sword_02b_pixels),
      playerAttackText(33, 286, "100", 0xFFFF, 2),

      playerDefenceIcon(99, 285, 16, 16, image_armor_01b_pixels),
      playerDefenceText(116, 286, "100", 0xFFFF, 2),

      playerHeartIcon(174, 285, 15, 16, image_cards_hearts_bits),
      playerHeartText(192, 286, "20", 0xFFFF, 2),

      ringSlotBox(57, 69, 32, 32),
      ringSlot2Box(57, 114, 32, 32),
      helmetSlotBox(106, 69, 32, 32),
      breastplateSlotBox(106, 114, 32, 32),
      legSlotBox(106, 161, 32, 32),

      bootSlotBox(106, 206, 32, 32),
      amuletSlotBox(156, 69, 32, 32),
      swordSlotBox(156, 114, 32, 32)
{
    // Register all elements
    addElement(&header);
    addElement(&playerAttackIcon);
    addElement(&playerAttackText);

    addElement(&playerDefenceIcon);
    addElement(&playerDefenceText);

    addElement(&playerHeartIcon);
    addElement(&playerHeartText);

    addElement(&ringSlotBox);
    addElement(&ringSlot2Box);
    addElement(&helmetSlotBox);
    addElement(&breastplateSlotBox);
    addElement(&legSlotBox);
    addElement(&bootSlotBox);
    addElement(&amuletSlotBox);
    addElement(&swordSlotBox);
}

void InventoryScreen::update(unsigned long now)
{
}
