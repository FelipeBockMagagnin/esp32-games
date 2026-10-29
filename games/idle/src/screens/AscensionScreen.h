#ifndef ASCENSION_SCREEN_H
#define ASCENSION_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../ui/StarRise.h"
#include "../game/GameState.h"
#include "../game/Ascension.h"
#include "../managers/SoundManager.h"

// Played between a confirmed ascension and the restart: the points the run earned count up
// under rising sparks, then the new total shows. A press skips ahead. Calls `done`, which
// banks the points and restarts, so it never returns.
class AscensionScreen : public Screen
{
public:
    AscensionScreen(const GameState &game, const Ascension &ascension, SoundManager &sound, void (*done)());

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override { skip(); }
    void onUpPress() override { skip(); }
    void onDownPress() override { skip(); }
    bool onBackPress() override
    {
        skip();
        return true;
    }

private:
    Text title;
    Text runText;
    Text gainText;
    Text gainLabel;
    Text totalText;
    Text busyText;
    StarRise stars;

    const GameState &game;
    const Ascension &ascension;
    SoundManager &sound;
    void (*done)();

    uint32_t gained;
    uint32_t shownGain;
    unsigned long start; // 0 until the first update, so the timeline starts with the screen
    bool finishing;

    void skip();
};

#endif // ASCENSION_SCREEN_H
