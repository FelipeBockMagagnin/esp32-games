#ifndef SHOP_SCREEN_H
#define SHOP_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "ListRow.h"
#include "ListView.h"
#include "../game/World.h"
#include "../game/Colony.h"
#include "../managers/SoundManager.h"

// Critters to adopt, seeds to learn and upgrades, in that order. Each species and upgrade
// waits for a colony of a certain size.
class ShopScreen : public Screen
{
public:
    ShopScreen(World &world, Colony &colony, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t ROW_COUNT = ListView::VISIBLE_ROWS;
    static const uint8_t SEED_COUNT = CROP_COUNT - 1; // Clover is known from the start
    static const uint8_t ENTRY_COUNT = SPECIES_COUNT + SEED_COUNT + UPGRADE_COUNT;

    Header header;
    Text coinsText;
    ListRow rows[ROW_COUNT];
    ListView list;

    World &world;
    Colony &colony;
    SoundManager &sound;

    void refresh();
    void fillRow(ListRow &row, uint8_t entry, bool selected);
};

#endif // SHOP_SCREEN_H
