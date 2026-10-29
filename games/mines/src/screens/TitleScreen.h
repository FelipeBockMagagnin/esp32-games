#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../mines/Field.h"
#include "../mines/Records.h"
#include "../ui/FieldView.h"

// The first screen: a small field that plays itself over and over, Play, the level, the theme
// and each level's best time and wins. The demo is drawn straight to the panel.
class TitleScreen : public Screen
{
public:
    // `settingsChanged` applies and stores the settings after the theme changes
    TitleScreen(TFT_eSPI &tft, Records &records, RandomSource random, void (*save)(), Settings &settings,
                void (*settingsChanged)(), ScreenManager &screenManager, SoundManager &sound, Screen &playScreen);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t DEMO_COLS = 14;
    static const uint8_t DEMO_ROWS = 6;
    static const uint8_t DEMO_CELLS = DEMO_COLS * DEMO_ROWS;
    static const uint8_t ROW_COUNT = 3;

    enum class Demo : uint8_t
    {
        WAITING,  // Covered, before the first open
        SOLVING,  // Opening and flagging in a shuffled order
        SWEEPING, // Cleared
        RESTING,
        COVERING
    };

    TFT_eSPI &tft;
    Records &records;
    RandomSource random;
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
    Text bestTimes[LEVEL_COUNT];
    Text bestWins[LEVEL_COUNT];

    Field demo;
    FieldView demoView;
    Demo phase = Demo::WAITING;
    uint8_t order[DEMO_CELLS];
    uint8_t next = 0;
    uint8_t step = 0;
    Cell raised = {0, 0}; // The flag last planted, drawn halfway up until the next move
    bool raising = false;
    unsigned long lastDemo = 0;

    uint8_t selected = 0;

    void refresh();
    void paint(TFT_eSPI &display);
    void restartDemo(unsigned long now);
    void advanceDemo();
};

#endif // TITLE_SCREEN_H
