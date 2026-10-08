#ifndef RESET_SCREEN_H
#define RESET_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/ListRow.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// Confirmation for wiping the save, opened as a modal by holding Up and Down in game.
// It defaults to No, so the gesture alone can never erase anything.
class ResetScreen : public Screen
{
private:
    Header header;

    Box warningCard;
    Text questionText;
    Text warningText;

    ListRow noRow;
    ListRow yesRow;

    Text hintText;
    Text resettingText;

    ScreenManager &screenManager;
    SoundManager &sound;
    bool yesSelected;
    bool resetting; // Set by Confirm on Yes; the next render shows it, then erases and restarts

    void refreshRows();

public:
    ResetScreen(ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void render(TFT_eSPI &tft) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
};

#endif // RESET_SCREEN_H
