#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"

// Shows the fight that CombatState is already running. Confirm lands a manual strike,
// Select raises the guard; both are on cooldowns shown at the bottom.
class CombatScreen : public Screen
{
private:
    Header header;

    Image enemySprite;
    Text enemyName;
    ProgressBar enemyHpBar;
    Image enemyAtkIcon;
    Text enemyAtkText;
    Image enemyDefIcon;
    Text enemyDefText;

    // Countdown to the next automatic attack, and what a manual strike multiplies it by
    Box timerBox;
    Text timerText;
    Text timerMultiplier;

    // Its own strip below the timer, never overlapping it: carries drop notifications
    // while they are fresh, otherwise whatever the fight is waiting on
    Text messageText;

    Image strikeIcon;
    Text strikeLabel;
    Text strikeState;
    Image guardIcon;
    Text guardLabel;
    Text guardState;

    Image playerHeartIcon;
    ProgressBar playerHpBar;

    CombatState &combat;
    SoundManager &sound;

    unsigned long lastRefresh;
    uint32_t seenKills;
    uint32_t seenDrops;
    unsigned long messageUntil; // A drop notification holds the line until this time

    void showFightElements(bool visible);
    void showDropMessage(unsigned long now);

public:
    CombatScreen(CombatState &combat, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onSelectPress() override;
};

#endif // COMBAT_SCREEN_H
