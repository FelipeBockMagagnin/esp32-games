#ifndef UPGRADE_SCREEN_H
#define UPGRADE_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../ui/Ellipse.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class UpgradeScreen : public Screen
{
private:
    Header header;

    // Gold balance and automatic production
    Ellipse goldIndicator;
    Text goldText;
    Text goldRate;

    // Upgrade slots
    Box slotBoxes[GOLD_UPGRADE_COUNT];
    Image slotIcons[GOLD_UPGRADE_COUNT];

    // Selected upgrade details
    Box detailBox;
    Text detailTitle;
    Text detailLine1;
    Text detailLine2;
    Ellipse priceIndicator;
    Text priceText;

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;
    uint8_t selectedUpgrade;

    void refreshDetails();

public:
    UpgradeScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Select button moves to the next upgrade, confirm button buys the selected one
    void onSelectPress() override;
    void onConfirmPress() override;
};

#endif // UPGRADE_SCREEN_H
