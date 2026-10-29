#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "../game/SaveGame.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// How long without a press before the backlight dims; 0 never does
static constexpr unsigned long DIM_AFTER_MS[] = {60000, 300000, 0};
static constexpr uint8_t DIM_OPTION_COUNT = sizeof(DIM_AFTER_MS) / sizeof(DIM_AFTER_MS[0]);
static const uint8_t DIM_DEFAULT = 1;

// A modal, opened by holding Left: sound, screen dimming, the theme, the way back to the game
// menu when the board has one, and the way to the reset question
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings; `toMenu` saves and
    // boots the game menu
    SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), ScreenManager &screenManager,
                   SoundManager &sound, Screen &resetScreen);

    void onEnter(TFT_eSPI &tft) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    static const uint8_t ROW_COUNT = 5;
    static const int16_t SWATCH_SIZE = 16;

    Header header;
    ListRow rows[ROW_COUNT];
    Text hintText;

    Settings &settings;
    void (*changed)();
    void (*toMenu)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    uint8_t selected;
    bool menuShown = false;
    // Byte-swapped RGB565 like the icon assets: four of the theme's colours, in the Theme row
    uint16_t swatch[SWATCH_SIZE * SWATCH_SIZE];

    void step(int8_t delta);
    void refreshRows();
    void paintSwatch();
};

#endif // SETTINGS_SCREEN_H
