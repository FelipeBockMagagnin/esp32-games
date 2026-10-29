#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../core/Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// Sound, screen dimming, the theme, and the way to the reset question; a screen of the rotation
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

private:
    static const uint8_t ROW_COUNT = 4;
    static const int16_t SWATCH_SIZE = 16;

    Header header;
    ListRow rows[ROW_COUNT];

    Settings &settings;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &resetScreen;
    uint8_t selected;
    // Byte-swapped RGB565 like the icon assets: four of the theme's colours, in the Theme row
    uint16_t swatch[SWATCH_SIZE * SWATCH_SIZE];

    void refreshRows();
    void paintSwatch();
};

#endif // SETTINGS_SCREEN_H
