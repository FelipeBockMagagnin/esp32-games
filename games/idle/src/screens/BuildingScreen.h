#ifndef BUILDING_SCREEN_H
#define BUILDING_SCREEN_H

#include "Screen.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/Ellipse.h"
#include "../ui/Text.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class BuildingScreen : public Screen
{
private:
    Header header;

    Ellipse goldIndicator;
    Text goldText;
    Text goldRate;

    ListRow rows[ListView::VISIBLE_ROWS];
    ListView list;

    // Which building each row currently shows, so the fields that never change for a
    // building are not rebuilt on every refresh
    uint8_t rowBuilding[ListView::VISIBLE_ROWS];

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;

public:
    BuildingScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Select button moves to the next building, confirm button buys the selected one
    void onSelectPress() override;
    void onConfirmPress() override;
};

#endif // BUILDING_SCREEN_H
