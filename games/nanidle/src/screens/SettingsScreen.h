#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "DialogScreen.h"
#include "Settings.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The last screen of the rotation, as a control panel: sound, screen dimming, the Windows
// theme, the way back to the game menu when the board has one, and the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings; `toMenu` saves and
    // boots the game menu; `reset` erases the save and restarts
    SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), void (*reset)(),
                   ScreenManager &screenManager, SoundManager &sound, DialogScreen &dialog);

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    enum Row : uint8_t
    {
        ROW_SOUND,
        ROW_DIM,
        ROW_THEME,
        ROW_MENU,
        ROW_RESET,
        ROW_COUNT
    };
    static const int16_t ROW_Y = 30;
    static const int16_t ROW_H = 30;

    struct SettingRow
    {
        FocusFrame frame;
        Text name;
        Text value;
    };

    Settings &settings;
    void (*changed)();
    void (*toMenu)();
    void (*reset)();
    ScreenManager &screenManager;
    SoundManager &sound;
    DialogScreen &dialog;
    SettingRow rows[ROW_COUNT];
    Text about;
    uint8_t selected = 0;
    bool menuShown = false;

    void step(int8_t delta);
    void refreshRows();
};

#endif // SETTINGS_SCREEN_H
