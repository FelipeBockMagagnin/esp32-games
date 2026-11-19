// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/app/Transport.h"
#include "../src/core/ScreenManager.h"
#include "../src/screens/CreditsScreen.h"
#include "../src/screens/LibraryScreen.h"
#include "../src/screens/PlayerScreen.h"
#include "../src/screens/SettingsScreen.h"
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
static Settings settings = {0, 1, 0};
static Player player;
static Playlist playlist(source);
static Turntable turntable;
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static Transport transport(player, playlist, nothing);
extern SettingsScreen settingsScreen;
static CreditsScreen creditsScreen(screens, (Screen &)settingsScreen);
SettingsScreen settingsScreen(settings, applyTheme, nothing, transport, screens, creditsScreen);
static LibraryScreen libraryScreen(transport, screens, settingsScreen);
static PlayerScreen playerScreen(transport, turntable, screens, libraryScreen);

static std::string outDir = ".";
static unsigned long ticked = 0;

// The player's 120 Hz runs off the same clock as the screens
static void run(unsigned long ms)
{
    for (unsigned long t = 0; t < ms; t += 8)
    {
        stubNow += 8;
        while (ticked * 1000 < (unsigned long long)stubNow * FRAME_RATE)
        {
            player.tick();
            ticked++;
        }
        transport.update();
        screens.update(stubNow);
    }
}

static void shot(const char *name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    stubNow = 1000;
    ticked = stubNow * FRAME_RATE / 1000;
    turntable.begin();
    screens.addScreen(&playerScreen, "Vitrola");
    run(500);
    shot("01_stopped");

    screens.handleConfirmPress();
    run(20000);
    shot("02_system_beeps");

    transport.play(ALBUMS[2].first);
    run(14000);
    shot("03_fur_elise");

    screens.handleRightPress();
    screens.handleUpPress();
    run(6000);
    shot("04_next_repeat");

    screens.handleConfirmPress();
    run(1500);
    shot("05_paused");
    screens.handleConfirmPress();

    screens.handleDownPress();
    run(200);
    shot("06_shelf");
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(200);
    shot("07_tracks");
    for (int i = 0; i < 12; i++)
    {
        screens.handleDownPress();
    }
    run(200);
    shot("08_tracks_scrolled");
    screens.handleConfirmPress();
    run(9000);
    shot("09_picked");

    screens.handleDownPress();
    screens.handleUpPress();
    run(100);
    shot("10_shelf_settings");
    screens.handleBackPress();

    for (uint8_t t = 1; t < THEME_COUNT; t++)
    {
        settings.theme = t;
        applyTheme();
        screens.setScreen(0);
        screens.repaint();
        playerScreen.onEnter(tft);
        run(3000);
        std::string name = "11_theme_" + std::to_string(t);
        shot(name.c_str());
    }
    settings.theme = 0;
    applyTheme();

    screens.openModal(&settingsScreen);
    run(200);
    shot("12_settings");
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(200);
    shot("13_credits");
    screens.handleBackPress();
    screens.handleBackPress();
    run(4000);
    shot("14_back");
    return 0;
}
