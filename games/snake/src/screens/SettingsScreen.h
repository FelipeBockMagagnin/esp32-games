#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The last screen of the rotation: sound, screen dimming, the way back to the game
// menu when the board has one, and the way to the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings; `toMenu` boots the
    // game menu
    SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), ScreenManager &screenManager,
                   SoundManager &sound, Screen &resetScreen);

    void onEnter(TFT_eSPI &tft) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t ROW_COUNT = 4;

    Header header;
    ListRow rows[ROW_COUNT];

    Settings &settings;
    void (*changed)();
    void (*toMenu)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    uint8_t selected;
    bool menuShown = false;

    void step(int8_t delta);
    void refreshRows();
};

#endif // SETTINGS_SCREEN_H
