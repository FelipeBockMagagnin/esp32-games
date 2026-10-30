#ifndef REBIRTH_SCREEN_H
#define REBIRTH_SCREEN_H

#include "Screen.h"
#include "DialogScreen.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../nan/Game.h"

// What a rebirth keeps and what it pays, and the button that asks before doing it
class RebirthScreen : public Screen
{
public:
    RebirthScreen(Game &game, SoundManager &sound, DialogScreen &dialog, void (*rebirth)());

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onConfirmPress() override;

private:
    static const uint8_t INFO_LINES = 5;
    static const uint8_t STAT_LINES = 5;

    Game &game;
    SoundManager &sound;
    DialogScreen &dialog;
    void (*rebirth)();

    WinPanel infoPanel;
    Text info[INFO_LINES];
    Text stats[STAT_LINES];
    WinButton button;

    void refresh();
};

#endif // REBIRTH_SCREEN_H
