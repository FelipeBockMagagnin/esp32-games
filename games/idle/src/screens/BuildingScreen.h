#ifndef BUILDING_SCREEN_H
#define BUILDING_SCREEN_H

#include "Screen.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"
#include "../ui/CoinDisplay.h"
#include "../ui/UpgradeRow.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class BuildingScreen : public Screen
{
private:
    Header header;

    CoinDisplay goldDisplay;
    UpgradeRow buildingRows[BUILDING_COUNT];

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;
    uint8_t selectedBuilding;

public:
    BuildingScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Select button moves to the next building, confirm button buys the selected one
    void onSelectPress() override;
    void onConfirmPress() override;
};

#endif // BUILDING_SCREEN_H
