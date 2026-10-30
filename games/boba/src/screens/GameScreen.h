#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../pet/Pet.h"
#include "../ui/Panel.h"

// The original's only game: guess which way the pet will turn, five times
class GameScreen : public Screen
{
public:
    GameScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
               SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onUpPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;

private:
    enum class Phase : uint8_t
    {
        ASK,
        REVEAL,
        SCORE
    };

    Pet &pet;
    LcdMatrix &lcd;
    Panel &panel;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Phase phase = Phase::ASK;
    uint8_t round = 0;
    uint8_t wins = 0;
    bool turnedLeft = false;
    bool won = false;
    bool second = false;
    unsigned long nextStepAt = 0;
    unsigned long phaseEndsAt = 0;

    void guess(bool left);
    void finish();
    void draw();
};

#endif // GAME_SCREEN_H
