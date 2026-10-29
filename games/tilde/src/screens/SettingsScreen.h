#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Settings.h"

// A modal, opened by holding Left: sound, screen dimming, the theme, passing a grown pet's
// egg on, the way back to the game menu when the board has one, and the way to the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings; `toMenu` saves and
    // boots the game menu, offered only while `hasMenu` says the board has one
    SettingsScreen(Display &display, Game &game, Settings &settings, void (*changed)(), void (*toMenu)(),
                   bool (*hasMenu)(), Screen &farewellScreen, Screen &resetScreen)
        : Screen(display), game(game), settings(settings), changed(changed), toMenu(toMenu), hasMenu(hasMenu),
          farewellScreen(farewellScreen), resetScreen(resetScreen)
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
    void (*toMenu)();
    bool (*hasMenu)();
    Screen &farewellScreen;
    Screen &resetScreen;
    uint8_t selected = 0;
    bool menuShown = false;

    void step(int8_t delta);
};

#endif // SETTINGS_SCREEN_H
