#ifndef UPGRADE_SCREEN_H
#define UPGRADE_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"
#include "../ui/CoinDisplay.h"
#include "../ui/UpgradeRow.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class UpgradeScreen : public Screen
{
private:
    Text titleText;

    Image headerLeftIcon;
    Text backText;

    Image headerRightIcon;
    Text nextText;

    CoinDisplay oreDisplays[ORE_COUNT];
    UpgradeRow upgradeRows[UPGRADE_COUNT];

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;
    uint8_t selectedUpgrade;

public:
    UpgradeScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Select button moves to the next upgrade, confirm button buys the selected one
    void onSelectPress() override;
    void onConfirmPress() override;
};

#endif // UPGRADE_SCREEN_H
