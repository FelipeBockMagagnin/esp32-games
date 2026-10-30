#ifndef AUGMENT_SCREEN_H
#define AUGMENT_SCREEN_H

#include "Screen.h"
#include "Allocation.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"

// The second home for energy: each augment fills its bar like a skill, but every level also
// costs gold, and a full bar waits for it
class AugmentScreen : public Screen
{
public:
    AugmentScreen(Game &game, Allocation &allocation, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    struct AugRow
    {
        FocusFrame frame;
        Text name;
        Text level;
        WinBar bar;
        Text effect;
        Text info;
    };

    static const uint8_t ROW_MODE = 0;
    static const uint8_t ROW_COUNT = AUGMENT_COUNT + 1;

    Game &game;
    Allocation &allocation;
    SoundManager &sound;

    Text energy;
    Text gold;
    WinButton modeButton;
    AugRow rows[AUGMENT_COUNT];
    Text hint;
    uint8_t selected = 1;

    bool act();
    void select(int8_t delta);
    void refresh();
};

#endif // AUGMENT_SCREEN_H
