// Plays a game on the PC: the real Arduboy2 port, host and game against tools/stub/, with the
// buttons pressed as a script says, on a virtual clock. Saves the panel as a PNG when the
// script ends. A crash, or AddressSanitizer catching a bad read or write, fails the run.
//
//   run <game index or slug> <out.png> [keys:ms]...
//   run list|settings <out.png> [theme index]   the screens around the games
//
// keys is any of U D L R (the arrows), O (OK), B (Left+Right, the other button) and M (Up+Down,
// held long enough opens the menu), or _ for none: "O:100 _:2000 R:500".
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "ButtonManager.h"
#include "Png.h"
#include "core/ScreenManager.h"
#include "play/Host.h"
#include "play/Sketch.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameListScreen.h"
#include "screens/SettingsScreen.h"
#include "ui/Themes.h"
#include "UI.h"
#include "ui/Theme.h"

static const int RIGHT_PIN = 19, CONFIRM_PIN = 27, UP_PIN = 26, DOWN_PIN = 23, LEFT_PIN = 22;

struct Step
{
    std::string keys;
    uint64_t untilUs;
};
static std::vector<Step> script;
static size_t current = 0;
uint64_t stubMicros = 0;

struct ScriptEnded
{
};

void stubStep(uint64_t us)
{
    stubMicros += us;
    while (current < script.size() && stubMicros >= script[current].untilUs)
    {
        current++;
    }
    if (current >= script.size())
    {
        throw ScriptEnded();
    }
}

int digitalRead(uint8_t pin)
{
    const std::string &k = current < script.size() ? script[current].keys : std::string();
    auto has = [&](char c) { return k.find(c) != std::string::npos; };
    bool down = false;
    switch (pin)
    {
    case UP_PIN: down = has('U') || has('M'); break;
    case DOWN_PIN: down = has('D') || has('M'); break;
    case LEFT_PIN: down = has('L') || has('B'); break;
    case RIGHT_PIN: down = has('R') || has('B'); break;
    case CONFIRM_PIN: down = has('O'); break;
    }
    return down ? LOW : HIGH;
}

static unsigned long seed = 1;
void randomSeed(unsigned long s) { seed = s ? s : 1; }
static unsigned long next()
{
    seed = seed * 1103515245 + 12345;
    return (seed >> 1) & 0x7fffffff;
}
long random(long max) { return max <= 0 ? 0 : (long)(next() % (unsigned long)max); }
long random(long min, long max) { return min >= max ? min : min + random(max - min); }
int Print::printf(const char *, ...) { return 0; }

struct Quit
{
};
static void quit(uint16_t) { throw Quit(); }

static void nothing() {}
static void play(uint16_t) {}

static int menus(const std::string &screen, const char *out, int theme)
{
    Settings settings = {0, 1, (uint8_t)theme};
    Theme::setActive(settings.theme);
    TFT_eSPI tft(DISPLAY_W, DISPLAY_H);
    SoundManager sound(32);
    ScreenManager screens(tft);
    ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every game's save and the", "settings will be erased.",
                               "Resetting..."},
                              Theme::BAD, nothing, screens, sound);
    GameListScreen listScreen("arduboy", play, sound);
    SettingsScreen settingsScreen(settings, nothing, nothing, screens, sound, resetScreen);
    screens.addScreen(&listScreen, "Games");
    screens.addScreen(&settingsScreen, "Settings");
    script.push_back({"_", 1000000});
    if (screen == "settings")
    {
        screens.setScreen(1);
    }
    for (int i = 0; i < 30; i++)
    {
        screens.update(millis());
    }
    writePng(out, tft.pixels.data(), tft.width(), tft.height());
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "run <game index or slug> <out.png> [keys:ms]...\n");
        return 2;
    }
    if (std::string(argv[1]) == "list" || std::string(argv[1]) == "settings")
    {
        return menus(argv[1], argv[2], argc > 3 ? atoi(argv[3]) : 0);
    }
    int index = -1;
    for (uint16_t i = 0; i < SKETCH_COUNT; i++)
    {
        if (std::to_string(i) == argv[1] || SKETCHES[i].slug == std::string(argv[1]))
        {
            index = i;
        }
    }
    if (index < 0)
    {
        fprintf(stderr, "No game %s\n", argv[1]);
        return 2;
    }
    uint64_t at = 0;
    for (int i = 3; i < argc; i++)
    {
        std::string arg = argv[i];
        size_t colon = arg.find(':');
        at += 1000ULL * (colon == std::string::npos ? 1000 : std::stoul(arg.substr(colon + 1)));
        script.push_back({arg.substr(0, colon), at});
    }
    // A last moment with no button, so the shot shows the game rather than the press
    script.push_back({"_", at + 50000});

    TFT_eSPI tft(DISPLAY_W, DISPLAY_H);
    Button up(UP_PIN, 15), down(DOWN_PIN, 15), left(LEFT_PIN, 15), right(RIGHT_PIN, 15), ok(CONFIRM_PIN, 15);
    SoundManager sound(32);
    Host host(tft, {up, down, left, right, ok}, "arduboy", sound, quit, quit);
    const char *ended = "script ended";
    try
    {
        host.start((uint16_t)index);
        SKETCHES[index].setup();
        while (true)
        {
            SKETCHES[index].loop();
        }
    }
    catch (ScriptEnded &)
    {
    }
    catch (Quit &)
    {
        ended = "quit";
    }
    writePng(argv[2], tft.pixels.data(), tft.width(), tft.height());
    printf("%s: %s after %.1f s\n", SKETCHES[index].slug, ended, stubMicros / 1e6);
    return 0;
}
