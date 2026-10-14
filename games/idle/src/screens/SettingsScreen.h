#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../game/SaveGame.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// How long without a press before the backlight dims; 0 never does
static constexpr unsigned long DIM_AFTER_MS[] = {60000, 300000, 0};
static constexpr uint8_t DIM_OPTION_COUNT = sizeof(DIM_AFTER_MS) / sizeof(DIM_AFTER_MS[0]);
static const uint8_t DIM_DEFAULT = 1;

// A modal, opened by holding Left: sound, screen dimming, and the way to the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings
    SettingsScreen(Settings &settings, void (*changed)(), ScreenManager &screenManager, SoundManager &sound,
                   Screen &resetScreen);

    void onEnter(TFT_eSPI &tft) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    static const uint8_t ROW_COUNT = 3;

    Header header;
    ListRow rows[ROW_COUNT];
    Text hintText;

    Settings &settings;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    uint8_t selected;

    void refreshRows();
};

#endif // SETTINGS_SCREEN_H
