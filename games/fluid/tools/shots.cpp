// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/fluid/Palettes.h"
#include "../src/fluid/Prefs.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/TitleScreen.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static TFT_eSPI tft;
static Prefs prefs;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "The palette, theme and", "settings go back to defaults.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static PlayScreen playScreen(tft, prefs, nothing, screens, sound);
static TitleScreen titleScreen(tft, prefs, nothing, settings, applyTheme, screens, sound, playScreen);
static SettingsScreen settingsScreen(settings, nothing, nothing, screens, sound, resetScreen);

static std::string outDir = ".";

static void run(unsigned long ms, uint8_t held = 0)
{
    playScreen.setHeld(held);
    for (unsigned long t = 0; t < ms; t += 11)
    {
        stubNow += 11;
        screens.update(stubNow);
    }
    playScreen.setHeld(0);
}

static void shot(const char *name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

static const uint8_t U = PlayScreen::HELD_UP, D = PlayScreen::HELD_DOWN, L = PlayScreen::HELD_LEFT,
                     R = PlayScreen::HELD_RIGHT;

// A loop round the screen with a few diagonals, as a hand stirring would
static void stir()
{
    run(700, U);
    run(500, U | R);
    run(600, R);
    run(900, D);
    run(500, D | L);
    run(800, L);
    run(500, U | L);
    run(400, U);
    run(300, R);
    run(400, D | R);
    run(300);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    defaultPrefs(prefs);
    stubNow = 1000;
    screens.addScreen(&titleScreen, "Fluid");
    screens.addScreen(&settingsScreen, "Settings");
    run(400);
    shot("01_title");

    screens.handleConfirmPress();
    run(300);
    shot("02_ink_start");
    stir();
    shot("03_ink_stirred");
    stir();
    run(200, R);
    shot("04_ink_swirls");
    run(2500);
    shot("05_ink_settling");
    screens.handleConfirmPress();
    run(100);
    shot("06_ink_menu");
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleRightPress();
    screens.handleRightPress();
    run(100);
    shot("07_menu_palette");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(300);
    shot("08_water_start");
    run(2000);
    shot("09_water_settled");
    screens.handleBackPress();
    run(500);
    shot("10_water_tilting");
    run(2500);
    shot("11_water_left");
    screens.handleUpPress();
    run(700);
    shot("12_water_falling_up");
    screens.handleDownPress();
    run(3000);
    screens.handleDownPress();
    run(250);
    shot("13_water_shaken");
    run(600);
    shot("14_water_splash");
    screens.handleConfirmPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(300);
    shot("15_title_after");

    screens.setScreen(1);
    run(200);
    shot("16_settings");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(200);
    shot("17_reset");
    screens.handleBackPress();
    run(200);

    for (uint8_t t = 1; t < THEME_COUNT; t++)
    {
        Theme::setActive(t);
        prefs.palette = t % PALETTE_COUNT;
        screens.setScreen(1);
        screens.setScreen(0);
        run(300);
        std::string name = "18_title_" + std::string(THEMES[t].name);
        shot(name.c_str());
    }
    Theme::setActive(0);
    for (uint8_t p = 0; p < PALETTE_COUNT; p++)
    {
        prefs.palette = p;
        prefs.mode = (uint8_t)Mode::INK;
        screens.setScreen(0);
        screens.openModal(&playScreen);
        stir();
        std::string name = "19_ink_" + std::string(PALETTES[p].name);
        shot(name.c_str());
        screens.closeModal();
    }
    return 0;
}
