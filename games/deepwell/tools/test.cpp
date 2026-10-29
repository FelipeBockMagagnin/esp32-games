// Logic tests on the PC: the floor generator over many seeds, combat, and the save.
// Build and run with tools/test.sh.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <queue>
#include "../src/rogue/Run.h"
#include "../src/rogue/Save.h"
#include <Preferences.h>

unsigned long millis() { return 0; }
long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static bool stairsReachable(const Map &map)
{
    bool seen[MAP_H][MAP_W] = {};
    std::queue<std::pair<int, int>> open;
    open.push({map.startX, map.startY});
    seen[map.startY][map.startX] = true;
    while (!open.empty())
    {
        auto [x, y] = open.front();
        open.pop();
        if (map.at(x, y) == TILE_STAIRS)
        {
            return true;
        }
        const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (auto &d : dirs)
        {
            int nx = x + d[0], ny = y + d[1];
            if (map.walkable(nx, ny) && !seen[ny][nx])
            {
                seen[ny][nx] = true;
                open.push({nx, ny});
            }
        }
    }
    return false;
}

static void testGenerator()
{
    static Run run, again;
    for (uint32_t seed = 1; seed <= 1000; seed++)
    {
        for (uint8_t floor = 1; floor <= 12; floor++)
        {
            Run::Snapshot snap = {};
            snap.seed = seed;
            snap.floor = floor;
            snap.level = 1;
            snap.hp = PLAYER_BASE_HP;
            run.load(snap);
            const Map &map = run.getMap();
            assert(map.roomCount >= 1);
            assert(stairsReachable(map));
            assert(!run.onStairs());
            for (uint8_t i = 0; i < Run::MAX_ENTITIES; i++)
            {
                const Entity &e = run.getEntity(i);
                if (e.kind == Entity::NONE)
                {
                    continue;
                }
                assert(map.at(e.x, e.y) == TILE_FLOOR);
                assert(e.x != run.getPlayerX() || e.y != run.getPlayerY());
            }
            again.load(snap);
            assert(memcmp(&run.getMap(), &again.getMap(), sizeof(Map)) == 0);
            for (uint8_t i = 0; i < Run::MAX_ENTITIES; i++)
            {
                assert(memcmp(&run.getEntity(i), &again.getEntity(i), sizeof(Entity)) == 0);
            }
        }
    }
    printf("generator: 12000 floors ok\n");
}

// The first step of a shortest walk to (tx, ty), found from the target backwards; WAIT when
// there is none. Enemies do not block it: bumping one attacks it. `steps` gets the walk's length.
static Action stepToward(const Run &run, int tx, int ty, int *steps = nullptr)
{
    const Map &map = run.getMap();
    int dist[MAP_H][MAP_W];
    for (auto &row : dist)
        for (int &d : row)
            d = -1;
    std::queue<std::pair<int, int>> open;
    open.push({tx, ty});
    dist[ty][tx] = 0;
    const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // Action order
    while (!open.empty())
    {
        auto [x, y] = open.front();
        open.pop();
        for (auto &d : dirs)
        {
            int nx = x + d[0], ny = y + d[1];
            if (map.walkable(nx, ny) && dist[ny][nx] < 0)
            {
                dist[ny][nx] = dist[y][x] + 1;
                open.push({nx, ny});
            }
        }
    }
    int px = run.getPlayerX(), py = run.getPlayerY();
    if (steps)
    {
        *steps = dist[py][px] < 0 ? 999 : dist[py][px];
    }
    for (int i = 0; i < 4; i++)
    {
        int nx = px + dirs[i][0], ny = py + dirs[i][1];
        if (map.walkable(nx, ny) && dist[ny][nx] >= 0 && dist[ny][nx] < dist[py][px])
        {
            return (Action)i;
        }
    }
    return Action::WAIT;
}

static int nearestEnemy(const Run &run)
{
    int best = -1, bestSteps = 99;
    for (uint8_t i = 0; i < Run::MAX_ENTITIES; i++)
    {
        const Entity &e = run.getEntity(i);
        int steps = abs(e.x - run.getPlayerX()) + abs(e.y - run.getPlayerY());
        if (e.kind == Entity::ENEMY && steps < bestSteps)
        {
            best = i;
            bestSteps = steps;
        }
    }
    return best;
}

static void testCombat()
{
    assert(damageThrough(3, 10) == 1);
    assert(damageThrough(10, 3) == 7);
    assert(damageThrough(4, 3) == 1);

    // Walks into enemies until something dies, which must happen: damage is never 0
    static Run run;
    run.start(42);
    uint16_t xpBefore = run.getXp();
    bool killed = false;
    for (int turn = 0; turn < 2000 && !run.isDead() && !killed; turn++)
    {
        int target = nearestEnemy(run);
        Action action = target < 0 ? Action::WAIT
                                   : stepToward(run, run.getEntity(target).x, run.getEntity(target).y);
        killed = (run.act(action) & EV_KILL) != 0;
    }
    assert(killed || run.isDead());
    if (killed)
    {
        assert(run.getXp() > xpBefore && run.getKills() == 1);
    }
    printf("combat ok (%s)\n", killed ? "kill" : "died");
}

// A greedy player: fights whatever is near, drinks under a third of its health, otherwise
// heads for the stairs. Prints the floors it dies on, as a rough measure of difficulty.
static void autoplay()
{
    static Run run;
    const int RUNS = 200;
    int floors[RUNS];
    int killers[ENEMY_COUNT + 1] = {};
    for (int r = 0; r < RUNS; r++)
    {
        run.start(1000 + r);
        for (int turn = 0; turn < 20000 && !run.isDead(); turn++)
        {
            if (run.getHp() * 3 < run.getMaxHp() && run.getPotions() > 0)
            {
                run.act(Action::DRINK);
                continue;
            }
            if (run.onStairs())
            {
                run.act(Action::DESCEND);
                continue;
            }
            // The nearest enemy by the walk there, if it is close enough to bother with
            Action action = Action::WAIT;
            int nearest = 999;
            for (uint8_t i = 0; i < Run::MAX_ENTITIES; i++)
            {
                const Entity &e = run.getEntity(i);
                int steps;
                Action toward = e.kind == Entity::ENEMY ? stepToward(run, e.x, e.y, &steps) : Action::WAIT;
                if (e.kind == Entity::ENEMY && steps <= WAKE_DISTANCE && steps < nearest)
                {
                    nearest = steps;
                    action = toward;
                }
            }
            if (nearest == 999)
            {
                for (int y = 0; y < MAP_H; y++)
                    for (int x = 0; x < MAP_W; x++)
                        if (run.getMap().at(x, y) == TILE_STAIRS)
                            action = stepToward(run, x, y);
            }
            run.act(action);
        }
        assert(run.isDead());
        killers[run.getKiller()]++;
        floors[r] = run.getFloor();
    }
    std::sort(floors, floors + RUNS);
    printf("autoplay: died on floor %d / %d / %d (10th / median / 90th percentile)\n", floors[RUNS / 10],
           floors[RUNS / 2], floors[RUNS * 9 / 10]);
    printf("autoplay: killed by");
    for (uint8_t i = 0; i < ENEMY_COUNT; i++)
    {
        printf(" %s %d", ENEMIES[i].name, killers[i]);
    }
    printf("\n");
}

static void testSave()
{
    Preferences::store.clear();
    SlotStore store("test");
    static Run run;
    Records records = {};
    run.start(7);
    run.act(Action::WAIT);

    SaveBlob blob, read;
    captureSave(blob, run, records);
    assert(writeSave(store, blob));
    assert(readSave(store, read));
    assert(memcmp(&blob, &read, sizeof(blob)) == 0);
    assert(read.run.floor == 1 && read.run.seed == 7);

    // A failed write keeps the previous save readable
    SaveBlob changed = blob;
    changed.records.runs = 5;
    Preferences::failWrites = true;
    assert(!writeSave(store, changed));
    Preferences::failWrites = false;
    SlotStore fresh("test");
    assert(readSave(fresh, read) && read.records.runs == 0);

    // The newer of the two slots wins
    assert(writeSave(fresh, changed));
    SlotStore third("test");
    assert(readSave(third, read) && read.records.runs == 5);

    // A torn slot falls back to the other one
    Preferences::store["save1"][3] ^= 0xFF;
    SlotStore fourth("test");
    assert(readSave(fourth, read) && read.records.runs == 0);

    // A loaded run resumes at the start of its floor
    static Run resumed;
    resumed.load(blob.run);
    assert(resumed.getFloor() == 1 && resumed.getPlayerX() == resumed.getMap().startX);
    printf("save ok\n");
}

int main()
{
    testGenerator();
    testCombat();
    testSave();
    autoplay();
    printf("all tests passed\n");
    return 0;
}
