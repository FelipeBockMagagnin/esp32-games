#ifndef BUILDING_SCREEN_H
#define BUILDING_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/Ellipse.h"
#include "../ui/Text.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class BuildingScreen : public Screen
{
private:
    // One row fewer than the default, to make room for the detail panel at the bottom
    static const uint8_t ROWS = ListView::VISIBLE_ROWS - 1;

    Header header;

    // Gold card: balance and production, as on the Mining screen
    Box goldCard;
    Ellipse goldCoin;
    Text goldText;
    Text goldRate;

    ListRow rows[ROWS];
    ListView list;

    // Which building each row currently shows, and whether it was revealed then, so the
    // fields that never change for a building are not rebuilt on every refresh
    uint8_t rowBuilding[ROWS];
    bool rowRevealed[ROWS];

    // Detail panel for the selected building: what buying it does, and its share of income
    Box detailPanel;
    ButtonBadge buyBadge;
    Text buyLabel;
    Text buyDetail;
    ProgressBar shareBar;

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;

    bool isRevealed(uint8_t id) const;
    void updateDetail();

public:
    BuildingScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Up / Down buttons navigate through buildings, confirm button buys the selected one
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // BUILDING_SCREEN_H
