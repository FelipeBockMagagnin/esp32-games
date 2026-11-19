#ifndef CREDITS_SCREEN_H
#define CREDITS_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "../core/ScreenManager.h"

// Who made the music. System Beeps is CC-BY 3.0, which asks for exactly this.
class CreditsScreen : public Screen
{
public:
    CreditsScreen(ScreenManager &screenManager, Screen &settings);

    void onEnter(TFT_eSPI &tft) override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override { return true; }

private:
    Header header;
    ScreenManager &screenManager;
    Screen &settings;
};

#endif // CREDITS_SCREEN_H
