#ifndef GAME_OVER_SCREEN_H
#define GAME_OVER_SCREEN_H

#include "Screen.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../rogue/Run.h"
#include "../rogue/Save.h"
#include "../core/SoundManager.h"

// The end of a run, opened as a modal by the death; Confirm starts the next one
class GameOverScreen : public Screen
{
public:
    GameOverScreen(const Run &run, const Records &records, SoundManager &sound, void (*newRun)());

    // Before opening it: whether the run just recorded set a new best score
    void setNewBest(bool best) { newBest = best; }

    void onEnter(TFT_eSPI &tft) override;
    void onConfirmPress() override;
    bool onBackPress() override { return true; }

private:
    Header header;
    Box card;
    Text floorText;
    Text killerText;
    Text scoreText;
    Text bestText;
    Text statsText;
    Text hintText;

    const Run &run;
    const Records &records;
    SoundManager &sound;
    void (*newRun)();
    bool newBest;
};

#endif // GAME_OVER_SCREEN_H
