#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "Text.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// A modal, opened from the title or the pause menu: sound, screen dimming, the theme, the way
// back to the game menu when the board has one, and the way to the reset question
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
    bool onBackPress() override;
    bool onRightPress() override { return true; }

    // Where Left goes back to: that modal, or with none the screen under the modals
    void setReturnTo(Screen *screen) { returnTo = screen; }

private:
    static const uint8_t ROW_COUNT = 5;

    Header header;
    ListRow rows[ROW_COUNT];
    Text hintText;

    Settings &settings;
    void (*changed)();
    void (*toMenu)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    Screen *returnTo = nullptr;
    uint8_t selected;
    bool menuShown = false;

    void step(int8_t delta);
    void refreshRows();
};

#endif // SETTINGS_SCREEN_H
