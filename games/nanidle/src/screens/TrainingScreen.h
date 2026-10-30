#ifndef TRAINING_SCREEN_H
#define TRAINING_SCREEN_H

#include "Screen.h"
#include "Allocation.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"
#include "../ui/Battery.h"

// Energy into the training skills: the battery, the action Confirm takes, one row per skill and
// the button that takes every allocation back
class TrainingScreen : public Screen
{
public:
    TrainingScreen(Game &game, Allocation &allocation, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    struct SkillRow
    {
        FocusFrame frame;
        Text name;
        Text level;
        WinBar bar;
        Text info;
    };

    static const uint8_t ROW_MODE = 0;
    static const uint8_t ROW_RECLAIM = SKILL_COUNT + 1;
    static const uint8_t ROW_COUNT = SKILL_COUNT + 2;

    Game &game;
    Allocation &allocation;
    SoundManager &sound;

    Battery battery;
    Text stats;
    WinButton modeButton;
    SkillRow rows[SKILL_COUNT];
    WinButton reclaimButton;
    uint8_t selected = 1;

    void select(int8_t delta);
    bool act();
    void refresh();
};

#endif // TRAINING_SCREEN_H
