// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/nan/Game.h"
#include "../src/screens/AdventureScreen.h"
#include "../src/screens/AugmentScreen.h"
#include "../src/screens/BossScreen.h"
#include "../src/screens/DialogScreen.h"
#include "../src/screens/InventoryScreen.h"
#include "../src/screens/MinerScreen.h"
#include "../src/screens/RebirthScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/SplashScreen.h"
#include "../src/screens/TrainingScreen.h"
#include "../src/screens/XpScreen.h"
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
static Game game(source);
static Allocation allocation;
static Settings settings = {0, 1, 1};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}

static DialogScreen dialog(screens, sound);
static SplashScreen splash(screens, sound);
static TrainingScreen training(game, allocation, sound);
static BossScreen boss(game, sound);
static XpScreen xp(game, sound);
static AdventureScreen adventure(game, sound);
static InventoryScreen inventory(game, sound);
static MinerScreen miners(game, sound);
static AugmentScreen augments(game, allocation, sound);
static RebirthScreen rebirth(game, sound, dialog, nothing);
static SettingsScreen settingsScreen(settings, applyTheme, nothing, nothing, screens, sound, dialog);

static std::string outDir = ".";

static void run(unsigned long ms, bool tick = true)
{
    for (unsigned long t = 0; t < ms; t += 20)
    {
        stubNow += 20;
        if (tick && t % 100 == 0)
        {
            game.tick(0.1);
        }
        screens.update(stubNow);
    }
}

static void shot(const std::string &name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

static void setTheme(uint8_t theme)
{
    settings.theme = theme;
    applyTheme();
    screens.repaint();
}

// A player a few hours in: the sewers open, some gear, a little gold
static void midGame()
{
    Game::Snapshot snap;
    game.capture(snap);
    snap.unlocked = (1u << FEATURE_COUNT) - 1;
    snap.bossesBeaten = 12;
    snap.bestBosses = 13;
    snap.skillLevel[0] = 1500000;
    snap.skillLevel[1] = 1400000;
    snap.skillLevel[2] = 38000;
    snap.skillLevel[3] = 12000;
    snap.skillAlloc[0] = 120;
    snap.skillAlloc[1] = 90;
    snap.skillAlloc[2] = 60;
    snap.skillProgress[2] = 140;
    snap.energy = 600;
    snap.xpBought[0] = 12;
    snap.xpBought[1] = 9;
    snap.xpBought[2] = 4;
    snap.xpBought[3] = 2;
    snap.xp = 37;
    snap.gold = 23456;
    snap.miners[0] = 14;
    snap.miners[1] = 6;
    snap.miners[2] = 1;
    snap.augmentLevel[0] = 4;
    snap.augmentLevel[1] = 3;
    snap.augmentAlloc[0] = 40;
    snap.augmentProgress[0] = 250;
    snap.augmentProgress[1] = 400;
    snap.nanPoints = 4;
    snap.rebirths = 1;
    snap.runSeconds = 3 * 3600 + 1200;
    snap.playSeconds = 14 * 3600;
    for (uint8_t i = 0; i < 9; i++)
    {
        snap.itemLevel[i] = 1 + i * 3;
    }
    snap.equipped[(uint8_t)Slot::HEAD] = 5;
    snap.equipped[(uint8_t)Slot::LEGS] = 6;
    snap.equipped[(uint8_t)Slot::FEET] = 7;
    snap.equipped[(uint8_t)Slot::WEAPON] = 4;
    snap.equipped[EQUIP_COUNT - 2] = 8;
    snap.zone = 2;
    game.restore(snap);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    applyTheme();
    screens.addScreen(&training, "Treino");
    screens.addScreen(&boss, "Boss");
    screens.addScreen(&xp, "XP");
    screens.addScreen(&adventure, "Aventura");
    screens.addScreen(&inventory, "Itens");
    screens.addScreen(&miners, "Minerar");
    screens.addScreen(&augments, "Aprimorar");
    screens.addScreen(&rebirth, "Renascer");
    screens.addScreen(&settingsScreen, "Config");

    screens.openModal(&splash);
    run(900, false);
    shot("00_splash");
    run(3000, false);

    midGame();
    static const char *const THEME_TAGS[] = {"98", "xp", "7"};
    for (uint8_t t = 0; t < THEME_COUNT; t++)
    {
        setTheme(t);
        std::string tag = std::string("_") + THEME_TAGS[t];
        screens.setScreen(0);
        run(400);
        shot("01_training" + tag);
        screens.setScreen(1);
        game.setFighting(true);
        run(1300);
        shot("02_boss" + tag);
        game.setFighting(false);
        screens.setScreen(3);
        run(2600);
        shot("04_adventure" + tag);
    }

    setTheme(1);
    screens.setScreen(2);
    run(300);
    shot("03_xp");
    screens.setScreen(4);
    run(300);
    shot("05_inventory");
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(300);
    shot("06_inventory_pick");
    screens.handleBackPress();
    screens.setScreen(5);
    run(300);
    shot("07_miners");
    screens.setScreen(6);
    run(300);
    shot("08_augments");
    screens.setScreen(7);
    run(300);
    shot("09_rebirth");
    screens.setScreen(8);
    run(300);
    shot("10_settings");

    const char *lines[DialogScreen::LINES] = {"Novo recurso liberado:", "Aventura e Itens", "",
                                              "Ele ja esta na barra."};
    dialog.open("NaN Idle", DialogIcon::INFO, lines, "OK", nullptr, nothing);
    run(300);
    shot("11_unlock");
    screens.closeModal();
    for (uint8_t t = 0; t < THEME_COUNT; t++)
    {
        setTheme(t);
        screens.setScreen(7);
        screens.handleConfirmPress();
        run(200);
        shot(std::string("12_rebirth_ask_") + THEME_TAGS[t]);
        screens.closeModal();
    }
    return 0;
}
