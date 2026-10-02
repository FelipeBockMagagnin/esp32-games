#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"

// Shows the fight that CombatState is already running. Three actions sit between the
// enemy block and the player block: Confirm strikes, Select guards, and Back smites once
// an amulet is equipped (without one, Back goes back a screen as it does everywhere else).
//
// Laid out as two mirrored blocks, enemy above and player below, each with a health bar
// and an attack countdown. Warm colors belong to the enemy, cool ones to the player, and
// the bars all share one width so they read as a stack.
class CombatScreen : public Screen
{
private:
    Header header;

    Text enemyName;
    Image enemySprite;
    ProgressBar enemyHpBar;
    ProgressBar enemyAttackBar; // Drains towards the enemy's next hit

    // Its own strip: carries drop notifications while they are fresh, otherwise
    // whatever the fight is waiting on
    Text messageText;

    Text strikeLabel;
    Text strikeState;
    Text guardLabel;
    Text guardState;
    Text smiteLabel;
    Text smiteState;

    ProgressBar playerAttackBar; // Hidden entirely when it cannot damage the enemy
    ProgressBar playerHpBar;
    // Both sides' raw numbers on one line: the player's sheet plus the enemy defense,
    // which is the only enemy stat the attack bars do not already express as damage
    Text statsLine;

    CombatState &combat;
    SoundManager &sound;

    unsigned long lastRefresh;
    uint32_t seenKills;
    uint32_t seenDrops;
    unsigned long messageUntil; // A drop notification holds the message line until this time

    void showFightElements(bool visible);
    void showDropMessage(unsigned long now);
    void refreshActionStates(unsigned long now, bool fighting);
    void refreshTimerBars(unsigned long now, bool fighting);

public:
    CombatScreen(CombatState &combat, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
};

#endif // COMBAT_SCREEN_H
