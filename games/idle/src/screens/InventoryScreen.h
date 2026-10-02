#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/ButtonBadge.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../game/Stats.h"
#include "../managers/SoundManager.h"

// Two lists on one screen, because three buttons leave no room for a cursor:
// Up / Down cycle the rows, Confirm opens a slot and then equips inside it.
// The panel under the list describes the selected row, and inside a slot compares
// each item against the one currently worn.
class InventoryScreen : public Screen
{
private:
    enum Mode : uint8_t
    {
        SLOT_LIST, // One row per equipment slot
        ITEM_LIST  // Owned items that fit the slot being inspected, with a Back row first
    };

    // Two rows fewer than the default, to make room for the detail panel
    static const uint8_t ROWS = ListView::VISIBLE_ROWS - 2;
    static const uint8_t STAT_COLUMNS = 4; // Attack, defense, max HP, gold bonus

    Header header;

    // Totals of everything equipped, plus the upgrade bonuses
    Box statsCard;
    Image attackIcon;
    Text attackText;
    Image defenseIcon;
    Text defenseText;
    Image hpIcon;
    Text hpText;

    ListRow rows[ROWS];
    ListView list;

    // Detail panel for the selected row
    Box detailPanel;
    Text panelTitle;
    Text panelRight;
    Text statColumns[STAT_COLUMNS];
    ButtonBadge actionBadge;
    Text actionLabel;
    Text hintText;

    GameState &game;
    Inventory &inventory;
    SoundManager &sound;

    Mode mode;
    EquipSlot inspecting;
    unsigned long lastRefresh;
    uint32_t lastRevision;
    bool rowsDirty; // Rows only change with the selection or the inventory, not with time

    void enterSlotList();
    void enterItemList(EquipSlot slot);
    void refreshRows();
    void refreshTotals();
    void refreshPanel();

    // Fills the stat columns: an item's own values, or with `compare` its difference
    // from `against`. Columns that are zero on both sides are hidden.
    void showStats(int8_t itemId, int8_t against, bool compare);
    void hideStats();

public:
    InventoryScreen(GameState &game, Inventory &inventory, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
};

#endif // INVENTORY_SCREEN_H
