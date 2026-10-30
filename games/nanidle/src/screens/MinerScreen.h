#ifndef MINER_SCREEN_H
#define MINER_SCREEN_H

#include "Screen.h"
#include "ShopRow.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"

// Machines that mine gold while the game runs, each one dearer than the last
class MinerScreen : public Screen
{
public:
    MinerScreen(Game &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    Game &game;
    SoundManager &sound;
    WinPanel wallet;
    Text balance;
    ShopRow rows[MINER_COUNT];
    Text hint;
    uint8_t selected = 0;

    bool buy();
    void refresh();
};

#endif // MINER_SCREEN_H
