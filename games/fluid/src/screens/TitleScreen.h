#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../fluid/Prefs.h"

// The first screen: a banner of the palette's inks in waves, the two modes, the palette and the
// theme, and what the buttons do. The banner is drawn straight to the panel.
class TitleScreen : public Screen
{
public:
    // `settingsChanged` applies and stores the settings after the theme changes
    TitleScreen(TFT_eSPI &tft, Prefs &prefs, void (*save)(), Settings &settings, void (*settingsChanged)(),
                ScreenManager &screenManager, SoundManager &sound, Screen &playScreen);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t ROW_COUNT = 4;

    TFT_eSPI &tft;
    Prefs &prefs;
    void (*save)();
    Settings &settings;
    void (*settingsChanged)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &playScreen;

    Header header;
    ListRow rows[ROW_COUNT]; // Ink, water, palette, theme
    Text help;

    uint8_t selected = 0;
    unsigned long lastWave = 0;
    uint8_t wavePhase = 0;

    void refresh();
    void paint(TFT_eSPI &display);
    void drawBanner(bool whole);
};

#endif // TITLE_SCREEN_H
