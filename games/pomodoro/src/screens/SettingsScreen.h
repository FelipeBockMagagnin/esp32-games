#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "../app/Storage.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// How long without a press before the backlight dims; 0 never does
static constexpr unsigned long DIM_AFTER_MS[] = {60000, 300000, 0};
static constexpr uint8_t DIM_OPTION_COUNT = sizeof(DIM_AFTER_MS) / sizeof(DIM_AFTER_MS[0]);
static const uint8_t DIM_DEFAULT = 1;

// Minutes, picked by index in Settings
static constexpr uint8_t FOCUS_OPTIONS[] = {15, 20, 25, 30, 45, 50, 60};
static constexpr uint8_t FOCUS_OPTION_COUNT = sizeof(FOCUS_OPTIONS);
static const uint8_t FOCUS_DEFAULT = 2;
static constexpr uint8_t BREAK_OPTIONS[] = {3, 5, 10};
static constexpr uint8_t BREAK_OPTION_COUNT = sizeof(BREAK_OPTIONS);
static const uint8_t BREAK_DEFAULT = 1;
static constexpr uint8_t LONG_BREAK_OPTIONS[] = {10, 15, 20, 30};
static constexpr uint8_t LONG_BREAK_OPTION_COUNT = sizeof(LONG_BREAK_OPTIONS);
static const uint8_t LONG_BREAK_DEFAULT = 1;

// A modal, opened by holding Left: the phase lengths, sound, dimming, the theme, and the way
// to the reset question
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
    static const uint8_t ROW_COUNT = 8;
    static const int16_t SWATCH_SIZE = 16;

    Header header;
    ListRow rows[ROW_COUNT];
    Text hintText;

    Settings &settings;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    uint8_t selected;
    // Byte-swapped RGB565 like an icon: four of the theme's colours, in the Theme row
    uint16_t swatch[SWATCH_SIZE * SWATCH_SIZE];

    void refreshRows();
    void paintSwatch();
};

#endif // SETTINGS_SCREEN_H
