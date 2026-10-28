// Checks on the game logic: every floor's map is well formed and can be finished without dying
// (the solver plays it), the stomp and boss rules hold along that route, a side hit kills and
// restarts at the start, and the records survive a save. Build and run with tools/sim/test.sh;
// exits non-zero on any failure.
#include <Arduino.h>
#include <Preferences.h>

#include "../../src/game/Stage.h"
#include "../../src/game/SaveGame.h"
#include "Solver.h"

unsigned long millis() { return 0; }
long random(long max) { return max <= 0 ? 0 : std::rand() % max; }
long random(long min, long max) { return min + random(max - min); }

static int failures = 0;

#define CHECK(cond)                                                       \
    do                                                                    \
    {                                                                     \
        if (!(cond))                                                      \
        {                                                                 \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static void testMaps()
{
    puts("maps");
    const char *legend = " #=-Xv^AESCWDPQBbKFM";
    for (uint8_t l = 0; l < LEVEL_COUNT; l++)
    {
        const LevelDef &def = LEVELS[l];
        size_t width = strlen(def.map[0]);
        CHECK(width <= Stage::MAX_COLS);
        int starts = 0, coworkers = 0, bosses = 0;
        for (uint8_t r = 0; r < MAP_ROWS; r++)
        {
            CHECK(strlen(def.map[r]) == width);
            for (const char *c = def.map[r]; *c != '\0'; c++)
            {
                CHECK(strchr(legend, *c) != nullptr);
                starts += *c == 'S';
                coworkers += *c == 'F';
                bosses += *c == 'K';
            }
        }
        CHECK(starts == 1);
        CHECK(coworkers == 1);
        CHECK(bosses <= 1);
        CHECK(def.dialogueLines % 2 == 0);
        for (uint8_t i = 0; i < def.dialogueLines; i++)
        {
            CHECK(strlen(def.dialogue[i]) <= 24);
        }
    }
}

// The route must reach the coworker alive, stomp on the way, and only ever hurt a dizzy boss
static void testRoute(uint8_t level)
{
    printf("route through %s %s\n", LEVELS[level].floor, LEVELS[level].name);
    std::vector<Input> route = solve(level, false);
    CHECK(!route.empty());

    Stage stage;
    stage.load(level);
    int stomps = 0;
    for (const Input &in : route)
    {
        bool dizzy = stage.isBossDizzy();
        uint8_t enemies = stage.aliveEnemies();
        stage.step(in);
        uint16_t events = stage.takeEvents();
        if (events & (EV_BOSS_HIT | EV_BOSS_DOWN))
        {
            CHECK(dizzy);
        }
        if ((events & EV_STOMP) && stage.aliveEnemies() < enemies)
        {
            stomps++;
        }
    }
    CHECK(stage.getPhase() == Stage::Phase::TALKING);
    CHECK(stage.getDeaths() == 0);
    CHECK(stomps > 0);
    if (LEVELS[level].boss != nullptr)
    {
        CHECK(stage.isBossDefeated());
    }
    printf("  %zu steps, %.1f s, %d stomps\n", route.size(), stage.getTicks() / (double)STEPS_PER_SECOND, stomps);
}

// Running into the first walker without jumping is a side hit: a death, then back to the start
static void testSideHit()
{
    puts("side hit");
    Stage stage;
    stage.load(0);
    float startX = stage.getPlayerX();
    Input run = {false, true, false, false, false};
    int steps = 0;
    while (!stage.isPlayerDead() && steps < 10 * STEPS_PER_SECOND)
    {
        stage.step(run);
        steps++;
    }
    CHECK(stage.isPlayerDead());
    CHECK(stage.getDeaths() == 1);
    CHECK(stage.takeEvents() & EV_DIE);
    Input none = {};
    for (int i = 0; i < 2 * STEPS_PER_SECOND; i++)
    {
        stage.step(none);
    }
    CHECK(!stage.isPlayerDead());
    CHECK(stage.getPlayerX() == startX);
}

// Falling onto a walker from above kills it and throws the player up, higher with jump held
static void testStomp()
{
    puts("stomp");
    for (int held = 0; held < 2; held++)
    {
        Stage stage;
        stage.load(0);
        // Running right, jumping at the first step a copy of the stage shows that jump landing on
        // the walker; the button stays held through the stomp only when asked to
        auto jumpInput = [&](int t)
        {
            Input in = {false, true, t == 0 || (held && t < 90), t == 0, false};
            return in;
        };
        auto jumpStomps = [&](const Stage &from)
        {
            Stage probe = from;
            for (int t = 0; t < STEPS_PER_SECOND && !probe.isPlayerDead(); t++)
            {
                probe.step(jumpInput(t));
                if (probe.takeEvents() & EV_STOMP)
                {
                    return true;
                }
            }
            return false;
        };
        bool stomped = false;
        int jumpedAt = -1;
        float bounce = 0;
        for (int i = 0; i < 10 * STEPS_PER_SECOND && !stage.isPlayerDead(); i++)
        {
            if (jumpedAt < 0 && stage.isPlayerOnFloor() && jumpStomps(stage))
            {
                jumpedAt = i;
            }
            Input in = jumpedAt >= 0 ? jumpInput(i - jumpedAt) : Input{false, true, false, false, false};
            stage.step(in);
            if ((stage.takeEvents() & EV_STOMP) && !stomped)
            {
                stomped = true;
                bounce = stage.getPlayerVY();
            }
        }
        CHECK(stomped);
        CHECK(!stage.isPlayerDead());
        // The prototype's BOUNCE_VELOCITY with jump held, a hop without
        CHECK(held ? bounce <= -600 : (bounce > -400 && bounce < -250));
    }
}

static void testRecords()
{
    puts("records");
    eraseSave();
    SaveBlob blob;
    newSave(blob);
    CHECK(recordClear(blob, 0, 3000, 4));
    CHECK(!recordClear(blob, 0, 3600, 1));
    CHECK(recordClear(blob, 0, 2400, 9));
    CHECK(blob.floors[0].bestTicks == 2400);
    CHECK(blob.floors[0].fewestDeaths == 1);
    CHECK(blob.floors[0].clears == 3);
    CHECK(saveGame(blob));

    SaveBlob loaded;
    CHECK(loadGame(loaded));
    CHECK(memcmp(&loaded, &blob, sizeof(blob)) == 0);

    blob.version = SAVE_VERSION + 1;
    CHECK(saveGame(blob));
    CHECK(loadGame(loaded));
    CHECK(loaded.version == SAVE_VERSION);

    eraseSave();
    CHECK(!loadGame(loaded));
}

int main()
{
    testMaps();
    for (uint8_t l = 0; l < LEVEL_COUNT; l++)
    {
        testRoute(l);
    }
    testSideHit();
    testStomp();
    testRecords();
    if (failures > 0)
    {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
