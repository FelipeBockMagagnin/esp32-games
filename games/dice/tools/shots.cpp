// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <deque>
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/dice/History.h"
#include "../src/dice/Prefs.h"
#include "../src/dice/Raffle.h"
#include "../src/dice/Spinner.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/DrawScreen.h"
#include "../src/screens/HistoryScreen.h"
#include "../src/screens/RollScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/WheelScreen.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

// The dice a scene wants, then a seeded stream
static std::deque<uint32_t> planned;
static uint32_t state = 0xC0FFEEu;
static uint32_t source()
{
    if (!planned.empty())
    {
        uint32_t v = planned.front();
        planned.pop_front();
        return v;
    }
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
// A raw value uniform() turns into `face` on an n-sided die
static void plan(std::initializer_list<int> faces, uint32_t sides)
{
    for (int f : faces)
    {
        planned.push_back((uint32_t)(f - 1) + sides * 1000);
    }
}

static TFT_eSPI tft;
static Prefs prefs;
static Raffle raffle;
static Spinner spinner;
static History history;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "The dice, ranges and wheel", "go back to the defaults.",
                                  "Erase and restart", "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static RollScreen rollScreen(prefs, history, source, sound, nothing);
static DrawScreen drawScreen(prefs, raffle, history, source, sound, nothing);
static WheelScreen wheelScreen(tft, prefs, spinner, history, source, sound, nothing);
static HistoryScreen historyScreen(history, sound);
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

static void shot(const char *name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

// Enters the screen again, so it reads the settings a scene changed behind its back
static void enter(int index)
{
    if (screens.getCurrentIndex() == index)
    {
        screens.setScreen((index + 1) % screens.getScreenCount());
    }
    screens.setScreen(index);
    run(200);
}

static void roll(RollSpec spec, std::initializer_list<int> faces, const char *name, unsigned long ms = 1000)
{
    prefs.roll = spec;
    enter(0);
    plan(faces, sidesOf(spec));
    screens.handleConfirmPress();
    run(ms);
    shot(name);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    defaultPrefs(prefs);
    stubNow = 1000;
    screens.addScreen(&rollScreen, "Roll");
    screens.addScreen(&drawScreen, "Draw");
    screens.addScreen(&wheelScreen, "Wheel");
    screens.addScreen(&historyScreen, "History");
    screens.addScreen(&settingsScreen, "Settings");
    run(200);
    shot("01_roll_ready");

    roll({2, 6, 5, (uint8_t)Keep::HIGHEST}, {7, 20}, "02_roll_critical");
    roll({4, 2, 0, (uint8_t)Keep::DROP_LOWEST}, {5, 2, 6, 4}, "03_roll_ability");
    roll({1, 6, 0, (uint8_t)Keep::SUM}, {1}, "04_roll_fumble");
    roll({8, 3, 2, (uint8_t)Keep::SUM}, {3, 8, 1, 5, 7, 2, 6, 4}, "05_roll_pool");
    roll({10, 7, 0, (uint8_t)Keep::SUM}, {100, 42, 7, 99, 13, 58, 1, 76, 31, 64}, "06_roll_percentile");
    roll({3, 1, 0, (uint8_t)Keep::SUM}, {2, 4, 3}, "07_roll_tumbling", 300);
    run(800);

    prefs.roll = {2, 2, 0, (uint8_t)Keep::SUM};
    enter(0);
    screens.handleUpPress();
    screens.handleUpPress();
    screens.handleConfirmPress();
    screens.handleUpPress();
    screens.handleUpPress();
    screens.handleUpPress();
    run(200);
    shot("08_roll_editing");
    screens.handleConfirmPress();

    prefs.drawFrom = 1;
    prefs.drawTo = 75;
    raffle.setRange(1, 75);
    enter(1);
    for (int i = 0; i < 9; i++)
    {
        screens.handleConfirmPress();
        run(800);
    }
    shot("09_draw_bingo");
    prefs.drawTo = 3;
    raffle.setRange(1, 3);
    enter(1);
    for (int i = 0; i < 3; i++)
    {
        screens.handleConfirmPress();
        run(800);
    }
    shot("10_draw_empty");

    prefs.slices = 8;
    spinner.setSlices(8);
    enter(2);
    shot("11_wheel_ready");
    screens.handleConfirmPress();
    run(1200);
    shot("12_wheel_spinning");
    run(4000);
    shot("13_wheel_result");
    prefs.slices = 12;
    spinner.setSlices(12);
    enter(2);
    screens.handleConfirmPress();
    run(5000);
    shot("14_wheel_twelve");

    enter(3);
    shot("15_history");
    enter(4);
    shot("16_settings");

    Theme::setActive(1);
    roll({2, 6, 5, (uint8_t)Keep::HIGHEST}, {7, 20}, "17_parchment_roll");
    enter(2);
    run(200);
    shot("18_parchment_wheel");
    Theme::setActive(0);
    return 0;
}
