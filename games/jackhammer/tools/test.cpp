// Logic tests on the PC: the square arena's fairness, the Unity difficulty curve, the shop and
// the save, then bots with a human's limits playing many rounds. Build and run with tools/test.sh.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>
#include "../src/jack/Round.h"
#include "../src/jack/Save.h"
#include <Preferences.h>

unsigned long millis() { return 0; }
long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static const int STEPS_PER_SECOND = 60;

static int firstAlive(const Round &round)
{
    for (uint8_t i = 0; i < Round::MAX_ENEMIES; i++)
    {
        if (round.getEnemy(i).alive)
        {
            return i;
        }
    }
    return -1;
}

// Facing each enemy's side the moment it spawns, every side must take as long to reach the head
static void testSquareArena()
{
    int stepsBySide[SIDE_COUNT] = {};
    for (uint32_t seed = 1; seed <= 200; seed++)
    {
        Round round;
        round.start(seed);
        while (firstAlive(round) < 0)
        {
            round.step();
        }
        uint8_t side = round.getEnemy(firstAlive(round)).side;
        while (round.getFacing() != side)
        {
            round.turn(1);
        }
        round.takeEvents();
        int steps = 0;
        while (!(round.takeEvents() & EVENT_KILL))
        {
            round.step();
            steps++;
            assert(!round.isDead());
        }
        assert(stepsBySide[side] == 0 || std::abs(stepsBySide[side] - steps) <= 1);
        stepsBySide[side] = steps;
    }
    for (int s = 1; s < SIDE_COUNT; s++)
    {
        assert(std::abs(stepsBySide[s] - stepsBySide[0]) <= 1);
    }
    float unity = (UNITY_SPAWN_DISTANCE - HEAD_REACH) / UNITY_ENEMY_SPEED;
    float here = stepsBySide[0] / (float)STEPS_PER_SECOND;
    assert(std::fabs(here - unity) < 0.05f);
    printf("Square arena: %.2f s from spawn to the head on every side (Unity: %.2f s on average, "
           "%.2f to %.2f by side)\n",
           here, unity, (7.0f - HEAD_REACH) / UNITY_ENEMY_SPEED, (9.5f - HEAD_REACH) / UNITY_ENEMY_SPEED);
}

// Turns at once toward the nearest enemy, as many times as it takes
static void aimNearest(Round &round)
{
    int target = -1;
    for (uint8_t i = 0; i < Round::MAX_ENEMIES; i++)
    {
        const Round::Enemy &e = round.getEnemy(i);
        if (e.alive && (target < 0 || e.distance < round.getEnemy(target).distance))
        {
            target = i;
        }
    }
    for (int turns = 0; target >= 0 && turns < 2 && round.getFacing() != round.getEnemy(target).side; turns++)
    {
        uint8_t diff = (round.getEnemy(target).side + SIDE_COUNT - round.getFacing()) % SIDE_COUNT;
        round.turn(diff == 3 ? -1 : 1);
    }
}

// DifficultyManager.increaseDifficulty, run once a second from the start
static void testDifficulty()
{
    Round round;
    round.start(7);
    float expected = START_INTERVAL;
    int second = 0;
    for (; second < 120 && !round.isDead(); second++)
    {
        expected -= expected >= FAST_UNTIL ? FAST_STEP : SLOW_STEP;
        round.step();
        assert(std::fabs(round.getInterval() - expected) < 1e-3f);
        for (int s = 1; s < STEPS_PER_SECOND && !round.isDead(); s++)
        {
            aimNearest(round);
            round.step();
        }
    }
    assert(second >= 60);
    printf("Difficulty: spawn interval %.2f s at the start, %.2f s at 10 s, %.2f s at %d s\n",
           START_INTERVAL - FAST_STEP, START_INTERVAL - 9 * FAST_STEP - 2 * SLOW_STEP, expected, second);
}

static void testRules()
{
    Round round;
    round.start(3);
    while (firstAlive(round) < 0)
    {
        round.step();
    }
    uint8_t side = round.getEnemy(firstAlive(round)).side;
    // Facing away from it, it reaches the body
    while (round.getFacing() == side)
    {
        round.turn(1);
    }
    while (!round.isDead())
    {
        round.step();
    }
    assert(round.getMarkCount() == 1 && round.getMark(0).side == side);
    assert(round.getPoints() == 0);
    // A dead round no longer turns
    uint8_t facing = round.getFacing();
    round.turn(1);
    assert(round.isDead() && round.getFacing() == facing);

    // Ten kills at x1, then x2
    round.start(4);
    while (round.getPoints() < 12)
    {
        aimNearest(round);
        round.step();
        assert(!round.isDead());
    }
    assert(round.getEarned() == 10 + 2 * 2 && round.getMultiplier() == 2);
    assert(round.takeUnbanked() == 14 && round.takeUnbanked() == 0);
}

static void testProfileAndSave()
{
    Profile p;
    newProfile(p);
    assert(owns(p, Shelf::STYLES, 0) && owns(p, Shelf::SONGS, 0));
    assert(!owns(p, Shelf::STYLES, 1) && !buyOrEquip(p, Shelf::STYLES, 1));
    assert(bankRound(p, 30, 250) && p.best == 30 && p.coins == 250);
    assert(!bankRound(p, 12, 0) && p.rounds == 2);
    assert(buyOrEquip(p, Shelf::STYLES, 1) && p.coins == 50 && p.style == 1);
    assert(buyOrEquip(p, Shelf::STYLES, 0) && p.coins == 50 && p.style == 0);
    assert(buyOrEquip(p, Shelf::STYLES, 1) && p.coins == 50);
    assert(!buyOrEquip(p, Shelf::SONGS, 1) && p.song == 0);

    SlotStore store("test");
    Profile loaded;
    assert(!readSave(store, loaded));
    assert(writeSave(store, p));
    assert(readSave(store, loaded) && memcmp(&loaded, &p, sizeof(p)) == 0);
    p.coins = 7;
    Preferences::failWrites = true;
    assert(!writeSave(store, p));
    Preferences::failWrites = false;
    SlotStore fresh("test");
    assert(readSave(fresh, loaded) && loaded.coins == 50);

    loaded.style = 200;
    sanitize(loaded);
    assert(loaded.style == 0);
}

struct BotResult
{
    uint32_t points;
    float seconds;
};

// Sees an enemy `reactMs` after it spawns, aims at the nearest one it has seen, and presses
// at most once every `pressGapMs`. Turns are counted, so the bot cannot hold a press.
static BotResult playBot(uint32_t seed, int reactMs, int pressGapMs)
{
    Round round;
    round.start(seed);
    int sinceLastPress = 1 << 20;
    float seen = SPAWN_DISTANCE - ENEMY_SPEED * reactMs / 1000.0f;
    while (!round.isDead() && round.getSeconds() < 600)
    {
        round.step();
        sinceLastPress += 1000 / STEPS_PER_SECOND;
        int target = -1;
        for (uint8_t i = 0; i < Round::MAX_ENEMIES; i++)
        {
            const Round::Enemy &e = round.getEnemy(i);
            if (e.alive && e.distance <= seen && (target < 0 || e.distance < round.getEnemy(target).distance))
            {
                target = i;
            }
        }
        if (target < 0 || sinceLastPress < pressGapMs)
        {
            continue;
        }
        uint8_t diff = (round.getEnemy(target).side + SIDE_COUNT - round.getFacing()) % SIDE_COUNT;
        if (diff != 0)
        {
            round.turn(diff == 3 ? -1 : 1);
            sinceLastPress = 0;
        }
    }
    return {round.getPoints(), round.getSeconds()};
}

static void reportBots()
{
    struct Player
    {
        const char *name;
        int reactMs;
        int pressGapMs;
    };
    static const Player PLAYERS[] = {
        {"Machine (no delay)", 0, 0},
        {"Sharp (180 ms, 8 presses/s)", 180, 125},
        {"Typical (250 ms, 6 presses/s)", 250, 166},
        {"Casual (350 ms, 4 presses/s)", 350, 250},
    };
    printf("\nBots, 300 rounds each: points (median / best), seconds survived (median)\n");
    for (const Player &player : PLAYERS)
    {
        std::vector<BotResult> results;
        for (uint32_t seed = 1; seed <= 300; seed++)
        {
            results.push_back(playBot(seed, player.reactMs, player.pressGapMs));
        }
        std::sort(results.begin(), results.end(), [](const BotResult &a, const BotResult &b)
                  { return a.points < b.points; });
        const BotResult &median = results[results.size() / 2];
        printf("  %-32s %5u / %5u   %6.1f s\n", player.name, (unsigned)median.points,
               (unsigned)results.back().points, median.seconds);
    }
}

int main()
{
    testSquareArena();
    testDifficulty();
    testRules();
    testProfileAndSave();
    reportBots();
    printf("\nAll tests passed\n");
    return 0;
}
