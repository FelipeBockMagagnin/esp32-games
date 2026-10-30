#ifndef ADVENTURE_SCREEN_H
#define ADVENTURE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"
#include "../ui/EnemySprite.h"

// The zones: Up/Down picks one, the auto-attack fights on its own, and Confirm strikes for
// three times the attack on a cooldown. Dying sends the player back to the safe zone.
class AdventureScreen : public Screen
{
public:
    AdventureScreen(Game &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    Game &game;
    SoundManager &sound;

    Text zoneName;
    WinPanel spriteFrame;
    EnemySprite sprite;
    Text enemyName;
    WinBar enemyBar;
    Text youLabel;
    WinBar playerBar;
    Text strikeLabel;
    WinBar strikeBar;
    Text rates;
    Text dropLine;
    Text tally;

    uint32_t seenSwings = 0;
    uint32_t seenEnemySwings = 0;
    uint32_t seenKills = 0;
    uint32_t seenDrops = 0;
    uint32_t seenDeaths = 0;
    const EnemyDef *shownEnemy = nullptr;
    uint8_t shownZone = 0xFF;

    void stepZone(int8_t delta);
    void refresh();
};

#endif // ADVENTURE_SCREEN_H
