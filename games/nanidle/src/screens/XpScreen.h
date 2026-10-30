#ifndef XP_SCREEN_H
#define XP_SCREEN_H

#include "Screen.h"
#include "ShopRow.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"

// Spends the XP the bosses and the adventure pay on the energy: cap, regeneration, power, and
// luck for the drops
class XpScreen : public Screen
{
public:
    XpScreen(Game &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    static const uint8_t ROW_COUNT = (uint8_t)XpUpgrade::COUNT;

    Game &game;
    SoundManager &sound;
    WinPanel wallet;
    Text balance;
    ShopRow rows[ROW_COUNT];
    Text hint;
    uint8_t selected = 0;

    bool buy();
    void refresh();
};

#endif // XP_SCREEN_H
