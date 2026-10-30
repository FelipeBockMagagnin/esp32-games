#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../stack/Game.h"
#include "../stack/Records.h"

// The first screen: a name built of blocks that drop in letter by letter, Play, the mode, the
// theme, each mode's best run and the controls. The logo is drawn straight to the panel.
class TitleScreen : public Screen
{
public:
    // `settingsChanged` applies and stores the settings after the theme changes
    TitleScreen(TFT_eSPI &tft, Records &records, void (*save)(), Settings &settings, void (*settingsChanged)(),
                ScreenManager &screenManager, SoundManager &sound, Screen &playScreen);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t ROW_COUNT = 3;
    static const uint8_t LETTER_COUNT = 7;

    TFT_eSPI &tft;
    Records &records;
    void (*save)();
    Settings &settings;
    void (*settingsChanged)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &playScreen;

    Header header;
    ListRow rows[ROW_COUNT]; // Play, mode, theme
    Text bestTitle;
    Text bestNames[MODE_COUNT];
    Text bestValues[MODE_COUNT];
    Text bestPlays[MODE_COUNT];
    Text controlLines[2];

    int8_t logoShown[LETTER_COUNT];
    uint8_t logoFrame = 0;
    unsigned long lastLogo = 0;

    uint8_t selected = 0;

    void refresh();
    void paint(TFT_eSPI &display);
    void drawLetter(uint8_t letter, int16_t lift);
};

#endif // TITLE_SCREEN_H
