#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Settings.h"

// A modal, opened by holding Left: sound, screen dimming, the theme, passing a grown pet's
// egg on, and the way to the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings
    SettingsScreen(Display &display, Game &game, Settings &settings, void (*changed)(), Screen &farewellScreen,
                   Screen &resetScreen)
        : Screen(display), game(game), settings(settings), changed(changed), farewellScreen(farewellScreen),
          resetScreen(resetScreen)
    {
    }

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

protected:
    const char *title() const override { return "Settings"; }
    void drawScene(Canvas &c) const override;

private:
    Game &game;
    Settings &settings;
    void (*changed)();
    Screen &farewellScreen;
    Screen &resetScreen;
    uint8_t selected = 0;
};

#endif // SETTINGS_SCREEN_H
