// Checks on the game logic that the balance simulator cannot see: the save slots, a new
// game from a zeroed save, and critters finding their way. Build and run with
// tools/sim/test.sh; exits non-zero on any failure.
#include <Arduino.h>
#include <Preferences.h>
#include <random>

#include "../../src/game/World.h"
#include "../../src/game/Colony.h"
#include "../../src/game/SaveGame.h"

static unsigned long simNow = 0;
static std::mt19937 rng(1);
unsigned long millis() { return simNow; }
long random(long max) { return max <= 0 ? 0 : (long)(rng() % (unsigned long)max); }
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

struct Game
{
    World world;
    Colony colony{world};

    void capture(SaveBlob &blob) const { captureSave(blob, world, colony); }
    void apply(const SaveBlob &blob) { applySave(blob, world, colony); }
};

static void run(Game &g, unsigned long ms)
{
    for (unsigned long end = simNow + ms; simNow < end; simNow += TICK_MS)
    {
        g.world.update(simNow);
        g.colony.update(simNow);
    }
}

static bool sameBlob(const SaveBlob &a, const SaveBlob &b)
{
    return memcmp(&a, &b, sizeof(SaveBlob)) == 0;
}

static void corruptSlotHolding(const SaveBlob &blob)
{
    for (auto &entry : Preferences::store)
    {
        if (entry.second.size() > sizeof(SaveBlob) && memcmp(entry.second.data(), &blob, sizeof(SaveBlob)) == 0)
        {
            entry.second[sizeof(SaveBlob) / 2] ^= 0xFF;
        }
    }
}

static void testRoundTrip()
{
    puts("save round trip");
    eraseSave();
    Game g;
    run(g, 5 * 60000);
    SaveBlob written, read;
    g.capture(written);
    CHECK(saveGame(written));
    CHECK(loadGame(read));
    CHECK(sameBlob(written, read));

    Game restored;
    restored.apply(read);
    SaveBlob again;
    restored.capture(again);
    CHECK(sameBlob(written, again));
}

// A failed write must leave the previous save loadable, and a torn newest slot falls back
static void testSlots()
{
    puts("save survives a failed write and a torn slot");
    eraseSave();
    Game g;
    SaveBlob first, second, read;
    g.capture(first);
    CHECK(saveGame(first));
    run(g, 60000);
    g.capture(second);
    CHECK(!sameBlob(first, second));

    Preferences::failWrites = true;
    CHECK(!saveGame(second));
    Preferences::failWrites = false;
    CHECK(loadGame(read) && sameBlob(read, first));

    CHECK(saveGame(second));
    CHECK(loadGame(read) && sameBlob(read, second));
    corruptSlotHolding(second);
    CHECK(loadGame(read) && sameBlob(read, first));
}

static void testZeroedSaveIsNewGame()
{
    puts("a zeroed snapshot is a new game");
    SaveBlob zero;
    memset(&zero, 0, sizeof(zero));
    Game fresh, loaded;
    loaded.apply(zero);
    CHECK(loaded.world.getCoins() == START_COINS);
    CHECK(loaded.colony.getCount() == fresh.colony.getCount());
    CHECK(loaded.world.count(TileKind::BURROW) == 1);
}

// Wild between a critter and the only trough: the way round is the only way, and a pond
// across it leaves none
static void testPathAroundWall()
{
    puts("a critter walks around a wall to the trough");
    World world;
    // Clear a U: the critter at the bottom of one arm, the trough at the bottom of the other
    World::Snapshot s;
    world.save(s);
    memset(s.kind, (uint8_t)TileKind::WILD, sizeof(s.kind));
    for (uint8_t y = 0; y <= 6; y++)
    {
        s.kind[World::indexOf(2, y)] = (uint8_t)TileKind::GRASS;
        s.kind[World::indexOf(6, y)] = (uint8_t)TileKind::GRASS;
    }
    for (uint8_t x = 2; x <= 6; x++)
    {
        s.kind[World::indexOf(x, 0)] = (uint8_t)TileKind::GRASS;
    }
    s.kind[World::indexOf(6, 6)] = (uint8_t)TileKind::TROUGH;
    world.load(s);

    uint8_t x = 2, y = 6, sx, sy;
    unsigned steps = 0;
    while (world.findPath(TileKind::TROUGH, x, y, sx, sy) && (sx != x || sy != y) && steps < 100)
    {
        x = sx;
        y = sy;
        steps++;
    }
    CHECK(x == 6 && y == 6);
    CHECK(steps == 16);

    s.kind[World::indexOf(4, 0)] = (uint8_t)TileKind::POND;
    world.load(s);
    CHECK(!world.findPath(TileKind::TROUGH, 2, 6, sx, sy));
}

static void testRegrow()
{
    puts("a harvested crop grows back without replanting");
    Game g;
    uint16_t soil = 0;
    while (g.world.kindAt(soil) != TileKind::SOIL)
    {
        soil++;
    }
    CHECK(g.world.plant(soil, 0));
    for (int i = 0; i < 400 && !g.world.isRipe(soil); i++)
    {
        g.world.water(soil);
        run(g, 1000);
    }
    CHECK(g.world.isRipe(soil));
    uint32_t food = g.world.getFood();
    CHECK(g.world.harvest(soil) || g.world.getFood() > food);
    CHECK(g.world.kindAt(soil) == TileKind::CROP);
    CHECK(!g.world.isRipe(soil));
}

int main()
{
    testRoundTrip();
    testSlots();
    testZeroedSaveIsNewGame();
    testPathAroundWall();
    testRegrow();
    if (failures > 0)
    {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    puts("all passed");
    return 0;
}
