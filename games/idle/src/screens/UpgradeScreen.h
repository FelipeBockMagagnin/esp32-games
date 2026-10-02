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
public:
    // The grid holds two rows of six; upgrades beyond that wait until a slot frees up
    static const uint8_t MAX_VISIBLE_SLOTS = 12;

private:
    Header header;

    // Gold balance and automatic production
    Ellipse goldIndicator;
    Text goldText;
    Text goldRate;

    // Upgrade slots
    Box slotBoxes[MAX_VISIBLE_SLOTS];
    Image slotIcons[MAX_VISIBLE_SLOTS];

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

    // Only unlocked, unbought upgrades get a slot, so the grid stays small as the
    // upgrade table grows. selectedSlot indexes this list, not GOLD_UPGRADES.
    uint8_t visibleIds[MAX_VISIBLE_SLOTS];
    uint8_t visibleCount;
    uint8_t selectedSlot;

    void rebuildVisible();
    void refreshDetails();

public:
    UpgradeScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Up / Down buttons navigate through upgrades, confirm button buys the selected one
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // UPGRADE_SCREEN_H
