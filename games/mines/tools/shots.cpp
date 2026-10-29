// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/mines/Field.h"
#include "../src/mines/Records.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/TitleScreen.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0xC0FFEEu;
static uint32_t source()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static bool confirmHeld = false;
static bool confirmDown() { return confirmHeld; }

static TFT_eSPI tft;
static Records records;
static Field field;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every best time, win and", "setting will be erased.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static PlayScreen playScreen(tft, field, records, source, nothing, confirmDown, screens, sound);
static TitleScreen titleScreen(tft, records, source, nothing, settings, applyTheme, screens, sound, playScreen);
static SettingsScreen settingsScreen(settings, nothing, nothing, screens, sound, resetScreen);

static std::string outDir = ".";

static void run(unsigned long ms)
{
    for (unsigned long t = 0; t < ms; t += 16)
    {
        stubNow += 16;
        screens.update(stubNow);
    }
}

static void shot(const std::string &name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

static void tap(Cell c)
{
    playScreen.moveTo(c);
    confirmHeld = true;
    screens.handleConfirmPress();
    run(48);
    confirmHeld = false;
    run(32);
}

static void hold(Cell c, unsigned long ms = 500)
{
    playScreen.moveTo(c);
    confirmHeld = true;
    screens.handleConfirmPress();
    run(ms);
    confirmHeld = false;
    run(32);
}

// Flags up to `count` mines that touch an open number and opens up to `opens` safe cells that
// do, as a player reading the field would
static void play(uint8_t flags, uint8_t opens)
{
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            Cell c = {x, y};
            if (field.isOpen(c) || field.isFlagged(c))
            {
                continue;
            }
            Cell around[8];
            uint8_t n = field.neighbours(c, around);
            bool edge = false;
            for (uint8_t i = 0; i < n; i++)
            {
                edge |= field.isOpen(around[i]);
            }
            if (!edge)
            {
                continue;
            }
            if (field.isMine(c) && flags > 0)
            {
                hold(c);
                flags--;
            }
            else if (!field.isMine(c) && opens > 0)
            {
                tap(c);
                run(600);
                opens--;
            }
        }
    }
}

static void clearAll()
{
    for (uint8_t y = 0; y < field.rows() && !field.over(); y++)
    {
        for (uint8_t x = 0; x < field.cols() && !field.over(); x++)
        {
            Cell c = {x, y};
            if (!field.isMine(c) && !field.isOpen(c))
            {
                tap(c);
                run(400);
            }
        }
    }
}

static Cell someMine()
{
    for (uint8_t y = field.rows(); y-- > 0;)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            if (field.isMine({x, y}) && !field.isFlagged({x, y}))
            {
                return {x, y};
            }
        }
    }
    return {0, 0};
}

static void startGame(uint8_t level)
{
    records.level = level;
    screens.setScreen(0);
    screens.handleConfirmPress();
    run(100);
}

static void quitGame()
{
    playScreen.moveTo({0, 0});
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(50);
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(100);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    defaultRecords(records);
    records.best[0] = 412;
    records.wins[0] = 6;
    records.plays[0] = 9;
    records.best[1] = 1873;
    records.wins[1] = 2;
    records.plays[1] = 11;
    records.level = 1;
    stubNow = 1000;
    screens.addScreen(&titleScreen, "Mines");
    screens.addScreen(&settingsScreen, "Settings");
    run(400);
    shot("01_title");
    run(3500);
    shot("02_title_demo");

    screens.handleConfirmPress();
    run(500);
    shot("03_start");
    Cell middle = playScreen.getCursor();
    tap(middle);
    run(60);
    shot("04_flood");
    run(1500);
    shot("05_opened");
    play(4, 3);
    run(400);
    shot("06_playing");
    Cell c = someMine();
    playScreen.moveTo(c);
    confirmHeld = true;
    screens.handleConfirmPress();
    run(300);
    shot("07_hold_ring");
    run(200);
    confirmHeld = false;
    run(32);
    run(200);
    shot("08_flagged");

    playScreen.moveTo({3, 0});
    screens.handleUpPress();
    run(100);
    shot("09_pause_button");
    screens.handleConfirmPress();
    run(100);
    shot("10_paused");
    screens.handleConfirmPress();
    run(100);

    tap(someMine());
    run(200);
    shot("11_blast");
    run(900);
    shot("12_chain");
    run(2500);
    shot("13_boom_box");
    screens.handleConfirmPress();
    run(200);

    tap(playScreen.getCursor());
    run(800);
    clearAll();
    run(150);
    shot("14_win_sweep");
    run(2500);
    shot("15_cleared");
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(200);
    shot("16_title_after");

    startGame(0);
    tap(playScreen.getCursor());
    run(1500);
    play(3, 2);
    shot("17_easy");
    quitGame();
    startGame(2);
    tap(playScreen.getCursor());
    run(1500);
    play(6, 4);
    shot("18_hard");
    quitGame();

    screens.setScreen(1);
    run(200);
    shot("19_settings");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(200);
    shot("20_reset");
    screens.handleBackPress();
    run(200);

    for (uint8_t t = 0; t < THEME_COUNT; t++)
    {
        settings.theme = t;
        applyTheme();
        state = 0xC0FFEEu + t;
        startGame(1);
        tap(playScreen.getCursor());
        run(1500);
        play(5, 3);
        playScreen.moveTo({5, 6});
        run(100);
        std::string name = std::string(THEMES[t].name);
        shot("21_play_" + name);
        tap(someMine());
        run(4000);
        shot("22_boom_" + name);
        screens.handleDownPress();
        screens.handleConfirmPress();
        run(2500);
        shot("23_title_" + name);
    }
    return 0;
}
