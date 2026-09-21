#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"

class CombatScreen : public Screen
{
private:
    // Header Navigation
    Image navMiningArrow;
    Text navMiningText;
    Image navSewersArrow;
    Text titleSewers;
    Text navSkillTreeText;
    Image navSkillTreeArrow;

    // Enemy / Central Sprite Section
    Image enemyRato;
    Text enemyName;

    // Enemy Stats Row
    Image swordIcon;
    Text swordStat;
    Image armorIcon;
    Text armorStat;
    Image enemyHeartIcon;
    Text enemyHp;

    // Countdown / Progress Section
    Box timerBox;
    Text timerText;
    Text timerMultiplierText;

    // Bottom Action / Player Section
    Image actionArmorIcon;
    Image actionOkBtn1;
    Text actionTimer1;
    Image actionSwordIcon;
    Image actionOkBtn2;
    Text actionTimer2;
    Image playerHeartIcon;
    Text playerHp;

    // Dynamic state
    static const unsigned long TIMER_PERIOD_MS = 3000;
    static const unsigned long TIMER_REFRESH_MS = 100;
    unsigned long timerDeadline;
    unsigned long lastTimerRefresh;

public:
    CombatScreen();

    void update(unsigned long now) override;
};

#endif // COMBAT_SCREEN_H
