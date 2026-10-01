#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../managers/SoundManager.h"

// Two lists on one screen, because three buttons leave no room for a cursor:
// Select cycles the rows, Confirm opens a slot and then equips inside it.
class InventoryScreen : public Screen
{
private:
    enum Mode : uint8_t
    {
        SLOT_LIST, // One row per equipment slot
        ITEM_LIST  // Owned items that fit the slot being inspected, with a Back row first
    };

    Header header;

    ListRow rows[ListView::VISIBLE_ROWS];
    ListView list;

    // Totals of everything equipped, plus the upgrade bonuses
    Image attackIcon;
    Text attackText;
    Image defenseIcon;
    Text defenseText;
    Image hpIcon;
    Text hpText;

    GameState &game;
    Inventory &inventory;
    SoundManager &sound;

    Mode mode;
    EquipSlot inspecting;
    unsigned long lastRefresh;
    uint32_t lastRevision;

    void enterSlotList();
    void enterItemList(EquipSlot slot);
    void refreshRows();
    void refreshTotals();

public:
    InventoryScreen(GameState &game, Inventory &inventory, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onSelectPress() override;
    void onConfirmPress() override;
};

#endif // INVENTORY_SCREEN_H
