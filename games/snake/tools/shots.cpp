// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/TitleScreen.h"
#include "../src/snake/Records.h"
#include "../src/snake/Snake.h"
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

static TFT_eSPI tft;
static Records records;
static Snake snake;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every best score and the", "settings will be erased.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static PlayScreen playScreen(tft, snake, records, source, nothing, screens, sound);
static TitleScreen titleScreen(tft, records, nothing, settings, applyTheme, screens, sound, playScreen);
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

static void press(Dir d)
{
    switch (d)
    {
    case Dir::UP:
        screens.handleUpPress();
        break;
    case Dir::DOWN:
        screens.handleDownPress();
        break;
    case Dir::LEFT:
        screens.handleBackPress();
        break;
    case Dir::RIGHT:
        screens.handleRightPress();
        break;
    }
}

static bool safe(Cell h, Dir d)
{
    int x = h.x + (d == Dir::RIGHT) - (d == Dir::LEFT), y = h.y + (d == Dir::DOWN) - (d == Dir::UP);
    if (x < 0 || x >= Snake::COLS || y < 0 || y >= Snake::ROWS)
    {
        return false;
    }
    return !snake.occupied({(uint8_t)x, (uint8_t)y});
}

// Plays like a careful beginner: at the food along x, then y, around its own body
static void autopilot(unsigned long ms)
{
    for (unsigned long t = 0; t < ms; t += 16)
    {
        Cell h = snake.segment(0), f = snake.food();
        Dir wanted[4] = {h.x < f.x ? Dir::RIGHT : Dir::LEFT, h.y < f.y ? Dir::DOWN : Dir::UP,
                         h.y < f.y ? Dir::UP : Dir::DOWN, h.x < f.x ? Dir::LEFT : Dir::RIGHT};
        if (h.x == f.x)
        {
            std::swap(wanted[0], wanted[1]);
        }
        for (Dir d : wanted)
        {
            if (d != opposite(snake.heading()) && safe(h, d))
            {
                if (d != snake.heading())
                {
                    press(d);
                }
                break;
            }
        }
        run(16);
    }
}

// Plays until the snake has eaten `count`, then waits for the food to sit right ahead
static void eatUntil(uint16_t count)
{
    while (snake.getEaten() < count)
    {
        autopilot(16);
    }
    for (int i = 0; i < 400; i++)
    {
        Cell a;
        if (snake.ahead(a) && a == snake.food())
        {
            break;
        }
        autopilot(16);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    defaultRecords(records);
    records.best[0] = 31;
    records.best[1] = 84;
    records.best[2] = 57;
    stubNow = 1000;
    screens.addScreen(&titleScreen, "Snake");
    screens.addScreen(&settingsScreen, "Settings");
    run(1200);
    shot("01_title");
    screens.handleDownPress();
    run(1000);
    shot("02_title_level");
    screens.handleConfirmPress();
    screens.handleUpPress();
    run(200);

    screens.handleConfirmPress();
    run(500);
    shot("03_ready");
    press(Dir::UP);
    eatUntil(14);
    shot("04_playing");
    while (snake.getEaten() < 15)
    {
        autopilot(16);
    }
    autopilot(4 * snake.getStepMs());
    shot("04_swallowed");
    screens.handleConfirmPress();
    run(100);
    shot("05_paused");
    screens.handleConfirmPress();
    run(100);
    shot("06_resumed");
    for (int i = 0; i < 2000 && playScreen.isPlaying(); i++)
    {
        autopilot(16);
        if (snake.getEaten() >= 40)
        {
            press(snake.heading() == Dir::UP || snake.heading() == Dir::DOWN ? Dir::LEFT : Dir::UP);
        }
    }
    run(300);
    shot("07_dying");
    run(1500);
    shot("08_game_over");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(200);
    shot("09_title_after");

    records.level = 0;
    screens.handleConfirmPress();
    run(200);
    press(Dir::UP);
    eatUntil(8);
    shot("10_easy_wrap");
    screens.handleConfirmPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(200);

    screens.setScreen(1);
    run(200);
    shot("11_settings");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(200);
    shot("11_reset");
    screens.handleBackPress();
    run(200);

    screens.setScreen(0);
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    screens.handleConfirmPress();
    run(1000);
    shot("12_title_theme");
    for (int i = 0; i < THEME_COUNT - 2; i++)
    {
        screens.handleConfirmPress();
    }
    screens.handleUpPress();
    screens.handleUpPress();
    run(200);
    screens.setScreen(1);

    for (uint8_t t = 1; t < THEME_COUNT; t++)
    {
        Theme::setActive(t);
        records.level = 1;
        screens.setScreen(0);
        screens.handleConfirmPress();
        run(200);
        press(Dir::UP);
        eatUntil(10);
        std::string name = "12_theme_" + std::string(THEMES[t].name);
        shot(name.c_str());
        screens.handleConfirmPress();
        screens.handleDownPress();
        screens.handleConfirmPress();
        run(1200);
        name = "13_title_" + std::string(THEMES[t].name);
        shot(name.c_str());
        screens.setScreen(1);
    }
    Theme::setActive(0);
    return 0;
}
