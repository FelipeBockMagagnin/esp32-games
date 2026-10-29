// Runs the real screens on the PC through scripted scenes and saves what the panel would show as a
// PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <cmath>
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/crawl/Save.h"
#include "../src/render/Frame.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/GameOverScreen.h"
#include "../src/screens/InventoryScreen.h"
#include "../src/screens/PauseScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/TitleScreen.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static TFT_eSPI tft(DISPLAY_W, DISPLAY_H);
static Run run;
static SaveBlob blob;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static Frame frame(tft);
static void nothing() {}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "The run, the records and the",
                                  "settings will be erased.", "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static ConfirmScreen abandonScreen({"Abandon", "Abandon the run?", "It ends here, as if you fell.", "",
                                    "Abandoning..."},
                                   Theme::BAD, nothing, screens, sound);
static SettingsScreen settingsScreen(settings, nothing, nothing, screens, sound, resetScreen);
static PlayScreen playScreen(run, frame, sound, {nothing, nothing, nothing, nothing});
static GameOverScreen gameOverScreen(screens, sound);
extern InventoryScreen inventoryScreen;
static PauseScreen pauseScreen(run, screens, sound, playScreen, inventoryScreen, settingsScreen, abandonScreen);
InventoryScreen inventoryScreen(run, screens, sound, pauseScreen);
static TitleScreen titleScreen(sound, blob, {nothing, nothing, nothing});

static std::string outDir = ".";

static void step(unsigned long ms, Input held = {})
{
    for (unsigned long t = 0; t < ms; t += 20)
    {
        stubNow += 20;
        playScreen.setHeld(held.forward, held.back, held.left, held.right, held.attack);
        screens.update(stubNow);
    }
}

static void shot(const char *name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), DISPLAY_W, DISPLAY_H);
}

// Turns the player toward the longest clear line of sight from where they stand
static void faceOpen()
{
    float best = 0, bestAngle = 0;
    for (int i = 0; i < 64; i++)
    {
        float a = i * 6.2831853f / 64;
        float d = 0;
        while (d < 12 && !run.getMap().solidAt(run.getX() + cosf(a) * d, run.getY() + sinf(a) * d))
        {
            d += 0.1f;
        }
        if (d > best)
        {
            best = d;
            bestAngle = a;
        }
    }
    run.place(run.getX(), run.getY(), bestAngle);
}

static void clearEnemies()
{
    for (uint8_t i = 0; i < Run::MAX_ENEMIES; i++)
    {
        run.enemyAt(i)->state = EnemyState::DEAD;
    }
}

static void ahead(float dist, float side, float &x, float &y)
{
    float a = run.getAngle();
    x = run.getX() + cosf(a) * dist - sinf(a) * side;
    y = run.getY() + sinf(a) * dist + cosf(a) * side;
}

static void startPlay(uint32_t seed)
{
    run.newRun(seed);
    clearEnemies();
    faceOpen();
    if (screens.hasModal())
    {
        screens.replaceModal(&playScreen);
    }
    else
    {
        screens.openModal(&playScreen);
    }
    playScreen.begin(false);
    step(100);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    srand(7);
    blob.records = {6, 41, 9, {}};
    blob.run.floor = 4;
    screens.addScreen(&titleScreen, "Grimhold");
    screens.update(0);
    shot("01_title");

    startPlay(21);
    step(200);
    shot("02_corridor");

    // The first floor's tutorial, once the floor's own notice has gone: turning taught, the sword next
    run.newRun(21);
    clearEnemies();
    playScreen.begin(true);
    screens.replaceModal(&playScreen);
    step(40);
    Input walk = {};
    walk.forward = true;
    step(600, walk);
    Input turn = {};
    turn.right = true;
    step(700, turn);
    step(2600);
    shot("02_tutorial");
    startPlay(21);

    // A fight: a skeleton up close, a goblin behind it, a frog mage casting from the back
    float x, y;
    ahead(1.6f, 0.2f, x, y);
    run.spawnEnemy(ENEMY_SKELETON, x, y)->state = EnemyState::CHASE;
    ahead(2.6f, -0.6f, x, y);
    run.spawnEnemy(ENEMY_GOBLIN, x, y);
    ahead(3.6f, 0.7f, x, y);
    run.spawnEnemy(ENEMY_FROG_WIZARD, x, y);
    Input charge = {};
    charge.attack = true;
    step(900, charge);
    shot("03_charging");
    step(100);
    step(120);
    shot("04_spell");

    // Something worth wearing at the player's feet
    startPlay(33);
    Item helm = {SLOT_HELM, 2, 1, 0, 34};
    run.dropItem(run.getX(), run.getY(), helm);
    ahead(1.4f, -0.3f, x, y);
    run.dropItem(x, y, Item{}, 1);
    ahead(2.2f, 0.4f, x, y);
    Item sword = {SLOT_WEAPON, 3, 2, 0, 30};
    run.dropItem(x, y, sword);
    step(200);
    shot("05_equipped");

    Input tap = {};
    tap.attack = true;
    ahead(1.1f, 0, x, y);
    run.spawnEnemy(ENEMY_ZOMBIE, x, y)->state = EnemyState::WINDUP;
    step(20, tap);
    step(60);
    shot("06_swing");

    run.inventory.bag[0] = {SLOT_WEAPON, 1, 0, 0, 14};
    run.inventory.bag[1] = {SLOT_RING, 2, 1, 0, 22};
    run.inventory.bag[2] = {SLOT_AMULET, 4, 2, 0, 18};
    run.inventory.bag[3] = {SLOT_BOOTS, 0, 0, 0, 5};
    run.inventory.equipped[SLOT_FOCUS] = {SLOT_FOCUS, 2, 1, 0, 26};
    run.inventory.equipped[SLOT_ARMOR] = {SLOT_ARMOR, 1, 0, 0, 4};
    run.inventory.hpPotions = 2;
    run.inventory.mpPotions = 1;
    screens.replaceModal(&pauseScreen);
    step(40);
    shot("07_pause");
    screens.replaceModal(&inventoryScreen);
    screens.handleDownPress();
    step(40);
    shot("08_inventory");

    // The lich's floor, the lich already awake
    run.newRun(5);
    while (!run.isBossFloor())
    {
        run.nextFloor();
    }
    clearEnemies();
    faceOpen();
    ahead(3.0f, 0, x, y);
    run.spawnEnemy(ENEMY_LICH, x, y, true)->state = EnemyState::CHASE;
    ahead(2.0f, 0.8f, x, y);
    run.spawnEnemy(ENEMY_WRAITH, x, y);
    screens.replaceModal(&playScreen);
    playScreen.begin(false);
    step(300);
    shot("09_lich");

    gameOverScreen.setResult(7, 38, true);
    screens.replaceModal(&gameOverScreen);
    step(40);
    shot("10_game_over");

    settingsScreen.setReturnTo(nullptr);
    screens.replaceModal(&settingsScreen);
    step(40);
    shot("11_settings");
    return 0;
}
