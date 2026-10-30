#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"

// The worn equipment, one row per place. Confirm opens the owned items for that place in the
// same rows; Confirm wears one, Left goes back.
class InventoryScreen : public Screen
{
public:
    InventoryScreen(Game &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRepeat(Key key) override;

private:
    static const uint8_t ROWS = EQUIP_COUNT;

    struct ItemRow
    {
        FocusFrame frame;
        Icon16 icon;
        Text caption;
        Text name;
        Text attack;
        Text defense;
    };

    Game &game;
    SoundManager &sound;
    ItemRow rows[ROWS];
    Text total;
    Text hint;

    bool picking = false;
    uint8_t place = 0;    // Selected place, or the one being filled while picking
    uint8_t choice = 0;   // Selected row while picking; 0 takes the item off
    int8_t choices[ROWS]; // Items listed while picking, after the empty row
    uint8_t choiceCount = 0;

    void fillRow(ItemRow &row, const char *caption, int8_t item, bool selected);
    void openPicker();
    void refresh();
};

#endif // INVENTORY_SCREEN_H
