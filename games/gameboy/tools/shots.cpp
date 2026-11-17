// Runs the real screens and games on the PC and saves what the panel would show as PNGs: the
// list, the settings, and each game named on the command line a few seconds in. Build and run
// with tools/shots.sh.
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/emu/Pack.h"
#include "../src/play/Player.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/GameListScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static TFT_eSPI tft;
static Pack pack;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void play(uint16_t) {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every game's save and the", "settings will be erased.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static GameListScreen listScreen(pack, "gameboy", play, sound);
static SettingsScreen settingsScreen(settings, applyTheme, nothing, screens, sound, resetScreen);
static Player player(tft, pack, "gameboy", sound, nothing, nothing);

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

// Presses Start now and then and A between, enough to get most games past their title
static void playFor(int frames)
{
    for (int i = 0; i < frames; i++)
    {
        stubNow += 17;
        bool start = i % 150 > 140, a = i % 90 > 84;
        Pad pad = {start, start, false, false, a, false, false, false};
        player.loop(pad);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: shots <pack.bin> <out_dir> [theme] [game index:frames]...\n");
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    static std::vector<uint8_t> blob((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!pack.open(blob.data(), blob.size()))
    {
        fprintf(stderr, "%s is not a pack\n", argv[1]);
        return 1;
    }
    outDir = argv[2];
    settings.theme = argc > 3 ? (uint8_t)atoi(argv[3]) : 0;
    applyTheme();

    tft.fillScreen(Theme::BG);
    screens.addScreen(&listScreen, "Games");
    screens.addScreen(&settingsScreen, "Settings");
    run(300);
    shot("01_list");
    for (int i = 0; i < 4; i++)
    {
        screens.handleDownPress();
    }
    run(300);
    shot("02_list_scrolled");
    screens.handleRightPress();
    run(300);
    shot("03_settings");

    for (int a = 4; a < argc; a++)
    {
        int index = atoi(argv[a]);
        const char *colon = strchr(argv[a], ':');
        int frames = colon != nullptr ? atoi(colon + 1) : 600;
        if (!player.start((uint16_t)index))
        {
            fprintf(stderr, "game %d refused\n", index);
            continue;
        }
        playFor(frames);
        std::string title = pack.entry((uint16_t)index).title;
        for (char &c : title)
        {
            c = isalnum((unsigned char)c) ? (char)tolower(c) : '_';
        }
        shot("game_" + std::to_string(index) + "_" + title);
    }
    return 0;
}
