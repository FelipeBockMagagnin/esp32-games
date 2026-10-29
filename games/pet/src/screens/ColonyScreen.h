#ifndef COLONY_SCREEN_H
#define COLONY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "ListRow.h"
#include "ListView.h"
#include "../game/World.h"
#include "../game/Colony.h"
#include "../managers/SoundManager.h"

// Every critter with what it is doing, its mood and what it earns. Confirm twice on one
// releases it, to make room for a better species.
class ColonyScreen : public Screen
{
public:
    ColonyScreen(const World &world, Colony &colony, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t ROW_COUNT = ListView::VISIBLE_ROWS;

    Header header;
    Text summaryText;
    Text incomeText;
    Text emptyText;
    ListRow rows[ROW_COUNT];
    ListView list;

    const World &world;
    Colony &colony;
    SoundManager &sound;
    bool armed; // The selected critter goes on the next Confirm
    uint32_t armedRevision;

    void refresh();
};

#endif // COLONY_SCREEN_H
