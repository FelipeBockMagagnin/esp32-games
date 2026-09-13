#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"

class HomeScreen : public Screen
{
private:
    // Header
    Text titleSewers;
    Text navInventory;
    Image navArrow;

    // Enemy Section
    Box enemyFrame;
    Image enemyRato;
    Text enemyName;

    // Enemy Stats Bar
    Box statsBar;
    Image swordIcon;
    Text swordStat;
    Image armorIcon;
    Text armorStat;
    Image enemyHeartIcon;
    Text enemyHp;

    // Combat Countdown
    Box timerBox;
    Text timerText;

    // Player Section
    Text levelText;
    Box expBox;
    Text expText;
    Image playerHeartIcon;
    Text playerHp;

    // Dynamic state
    unsigned long lastTimerUpdate;
    float timerCountdown;
    int currentEnemyHp;
    int currentPlayerHp;

public:
    HomeScreen();

    void update(unsigned long now) override;
};

#endif // HOME_SCREEN_H
