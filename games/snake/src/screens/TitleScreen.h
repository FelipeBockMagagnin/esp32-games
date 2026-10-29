#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../snake/Records.h"
#include "../snake/Snake.h"

// The first screen: the logo with a small snake crawling round it, Play, the level, the theme
// and the best score on each level. The crawler and the logo are drawn straight to the panel.
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
    static const uint8_t TRACK_COLS = 22;
    static const uint8_t TRACK_ROWS = 10;
    static const uint8_t TRACK_LENGTH = 2 * (TRACK_COLS + TRACK_ROWS) - 4;
    static const uint8_t CRAWLER_LENGTH = 7;
    static const uint8_t ROW_COUNT = 3;

    TFT_eSPI &tft;
    Records &records;
    void (*save)();
    Settings &settings;
    void (*settingsChanged)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &playScreen;

    Header header;
    ListRow rows[ROW_COUNT]; // Play, level, theme
    Text bestTitle;
    Text bestNames[LEVEL_COUNT];
    Text bestScores[LEVEL_COUNT];

    uint8_t selected = 0;
    uint8_t crawlerHead = 0; // Track index
    uint8_t foodAt = 0;
    bool fat[TRACK_LENGTH];
    unsigned long lastCrawl = 0;
    unsigned long nextBlink = 0;
    bool eyesShut = false;

    void refresh();
    void paint(TFT_eSPI &display);
    Cell trackCell(uint8_t i) const;
    Dir trackEntered(uint8_t i) const;
    uint8_t back(uint8_t i, uint8_t steps) const { return (uint8_t)((i + TRACK_LENGTH - steps) % TRACK_LENGTH); }
    bool onCrawler(uint8_t i) const;
    void placeFood();
    void drawLogo();
    void drawTrackCell(uint8_t i);
    void drawCrawler();
};

#endif // TITLE_SCREEN_H
