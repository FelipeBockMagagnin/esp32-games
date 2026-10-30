#ifndef BOSS_SCREEN_H
#define BOSS_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"
#include "../ui/EnemySprite.h"

// The boss ladder: the next boss over its zone's backdrop, both health bars, and Fight/Stop.
// The fight goes on in the background whatever screen is showing.
class BossScreen : public Screen
{
public:
    BossScreen(Game &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onConfirmPress() override;

private:
    Game &game;
    SoundManager &sound;

    Text name;
    WinPanel spriteFrame;
    EnemySprite sprite;
    WinBar bossBar;
    Text bossStats;
    Text youLabel;
    WinBar playerBar;
    Text rates;
    Text verdict;
    WinButton fightButton;
    Text record;

    uint32_t seenKills = 0;
    uint16_t shownBoss = 0xFFFF;
    bool wasFighting = false;
    double hpAtLastNumber = 0;
    unsigned long nextNumberAt = 0;

    void refresh();
};

#endif // BOSS_SCREEN_H
