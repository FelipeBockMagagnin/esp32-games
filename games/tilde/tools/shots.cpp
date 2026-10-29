// Runs the real screens on the PC through scripted scenes and saves what the panel would show:
// a PNG of each scene's last frame, and its frames for tools/shots.sh to join into a GIF.
// Build and run with tools/shots.sh [out_dir] [frame_ms].
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/pet/Album.h"
#include "../src/pet/Pet.h"
#include "../src/pet/Save.h"
#include "../src/screens/AlbumScreen.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/HatchScreen.h"
#include "../src/screens/HomeScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/SplashScreen.h"
#include "../src/screens/StatsScreen.h"
#include "../src/gfx/Themes.h"

static TFT_eSPI tft;
static Display display(tft);
static SlotStore store("shots");
static Pet pet(7);
static Album album;
static SoundManager sound(0);
static ScreenManager screens(tft);
static AutoSaver saver(store, pet, album);
static Game game = {pet, album, sound, screens, saver};
static Settings settings = {0, 1, 0};

static void nothing() {}
static void goPlay() { screens.setScreen(2); }
static HatchScreen hatch(display, game);
static HomeScreen home(display, game, hatch, goPlay);
static StatsScreen stats(display, game);
static PlayScreen play(display, game);
static AlbumScreen albumScreen(display, game);
static SplashScreen splash(display, game);
static ConfirmScreen confirm(display, game,
                             {"Reset", "Reset the game?", "The pet and the Album", "will be erased.", "Erase and restart"},
                             ALERT, nothing);
static SettingsScreen settingsScreen(display, game, settings, nothing, confirm, confirm);

static std::string outDir = ".";
// What a frame lasts: 66 by default, longer to see the game as a slow board runs it
static unsigned long FRAME_MS = 66;

// Runs the open screen for `ms`, frame by frame as the main loop does, ticking the pet each
// second; with a scene name it keeps every frame for a GIF
static void run(unsigned long ms, const std::string &scene = "")
{
    static unsigned long nextTick = 0;
    int frame = 0;
    if (!scene.empty())
    {
        mkdir((outDir + "/" + scene).c_str(), 0755);
    }
    for (unsigned long t = 0; t < ms; t += FRAME_MS)
    {
        stubNow += FRAME_MS;
        if ((long)(stubNow - nextTick) >= 0)
        {
            nextTick = stubNow + 1000;
            pet.tick();
            pet.takeEvents();
        }
        screens.update(stubNow);
        if (!scene.empty())
        {
            char name[16];
            snprintf(name, sizeof(name), "/%03d.png", frame++);
            writePng(outDir + "/" + scene + name, tft.pixels, TFT_WIDTH, TFT_HEIGHT);
        }
    }
}

static void shot(const std::string &name, unsigned long ms = 400, bool gif = false)
{
    run(ms, gif ? name : "");
    writePng(outDir + "/" + name + ".png", tft.pixels, TFT_WIDTH, TFT_HEIGHT);
    printf("%s\n", name.c_str());
}

static void become(FormId form, const char *name)
{
    Pet::Snapshot &raw = pet.raw();
    raw.form = form;
    raw.stageSecs = 0;
    pet.setName(name);
    for (auto &need : raw.needs)
    {
        need = 90;
    }
    raw.flags = 0;
    raw.fluff = 0;
    raw.bond = 320;
    raw.age = 100000;
    album.see(form);
    home.restart(stubNow);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    if (argc > 2 && atoi(argv[2]) > 0)
    {
        FRAME_MS = (unsigned long)atoi(argv[2]);
    }
    srand(3);
    stubNow = 1000;
    album.clear();
    screens.addScreen(&home, "Home");
    screens.addScreen(&stats, "Stats");
    screens.addScreen(&play, "Play");
    screens.addScreen(&albumScreen, "Album");

    screens.openModal(&splash);
    shot("00_splash", 2300, true);
    screens.closeModal();
    shot("01_egg", 4000, true);

    pet.raw().stageSecs = EGG_SECS;
    pet.tick();
    pet.takeEvents();
    shot("02_hatch", 3200, true);
    shot("03_name", 1500, true);
    screens.closeModal();

    become(FORM_BLIP, "Momo");
    shot("04_blip_idle", 9000, true);

    become(FORM_MOCHI, "Momo");
    pet.raw().fluff = 2;
    pet.raw().fluffSpot[0] = 3;
    pet.raw().fluffSpot[1] = 20;
    pet.raw().needs[NEED_FOOD] = 40;
    run(600);
    home.onConfirmPress();
    shot("05_mochi_meal", 2200, true);

    become(FORM_TUFT, "Pip");
    run(900);
    for (int i = 0; i < 4; i++)
    {
        home.onDownPress();
    }
    home.onConfirmPress();
    shot("06_tuft_cuddle", 1800, true);

    become(FORM_TUFT, "Pip");
    pet.raw().needs[NEED_JOY] = 10;
    pet.raw().needs[NEED_FOOD] = 15;
    pet.raw().flags = 1 << 2;
    shot("07_tuft_sick", 3000, true);

    become(FORM_NIMBUS, "Kiwi");
    for (auto &need : pet.raw().needs)
    {
        need = 100;
    }
    shot("08_nimbus_happy", 9000, true);

    become(FORM_PEBBLE, "Tofu");
    run(900);
    home.onConfirmPress();
    shot("09_pebble_refuse", 1200, true);

    become(FORM_PUDDLE, "Nori");
    pet.raw().needs[NEED_REST] = 30;
    run(900);
    pet.act(Action::LIGHT);
    shot("10_puddle_night", 5000, true);

    become(FORM_MOCHI, "Momo");
    run(600);
    pet.raw().form = FORM_SPROUT;
    shot("11_evolve", 2600, true);

    screens.setScreen(1);
    shot("12_stats", 600);
    screens.setScreen(2);
    shot("13_play_menu", 900, true);
    play.onConfirmPress();
    shot("14_hum", 4500, true);
    screens.setScreen(3);
    screens.setScreen(2);
    play.onDownPress();
    play.onConfirmPress();
    for (int i = 0; i < 40; i++)
    {
        run(130);
        i % 10 < 5 ? play.onBackPress() : play.onRightPress();
    }
    shot("15_catch", 2500, true);
    run(25000);
    shot("16_result", 1200, true);

    screens.setScreen(3);
    album.past[0] = {"Momo", FORM_PUDDLE};
    album.generation = 1;
    shot("17_album", 600);
    album.seen = (1 << FORM_EGG) | (1 << FORM_BLIP) | (1 << FORM_MOCHI);
    albumScreen.onDownPress();
    albumScreen.onDownPress();
    shot("18_album_unseen", 600);

    screens.setScreen(0);
    screens.openModal(&settingsScreen);
    shot("19_settings", 400);
    screens.replaceModal(&confirm);
    shot("20_confirm", 400);
    screens.closeModal();

    for (uint8_t i = 1; i < THEME_COUNT; i++)
    {
        Theme::setActive(i);
        become(FORM_MOCHI, "Momo");
        screens.repaint();
        shot(std::string("21_theme_") + THEMES[i].name, 1500);
    }
    return 0;
}
