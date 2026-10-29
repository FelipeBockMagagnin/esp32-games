// Logic tests for src/crawl/, run on the PC by tools/test.sh
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/crawl/Map.h"
#include "../src/crawl/Run.h"
#include "../src/crawl/Save.h"
#include "../src/crawl/Tutorial.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static int failures = 0;
#define CHECK(cond)                                                   \
    do                                                                \
    {                                                                 \
        if (!(cond))                                                  \
        {                                                             \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            failures++;                                               \
        }                                                             \
    } while (0)

static const Input IDLE_INPUT = {};

static void step(Run &run, const Input &input, int ms)
{
    for (int t = 0; t < ms; t += 20)
    {
        run.update(input, 20);
    }
}

// Every floor cell reaches the portal's side, and nothing spawns inside a wall
static void testFloors()
{
    for (uint32_t seed = 1; seed <= 2000; seed++)
    {
        Run run;
        run.newRun(seed);
        for (uint8_t f = 0; f < 3; f++)
        {
            const Map &map = run.getMap();
            CHECK(map.roomCount >= 2);
            CHECK(!map.solidAt(run.getX(), run.getY()));
            CHECK(map.at(map.portalX, map.portalY) == CELL_PORTAL);

            static bool seen[MAP_H][MAP_W];
            memset(seen, 0, sizeof(seen));
            static uint16_t queue[MAP_W * MAP_H];
            uint16_t head = 0, tail = 0;
            int16_t sx = (int16_t)run.getX(), sy = (int16_t)run.getY();
            queue[tail++] = (uint16_t)(sy * MAP_W + sx);
            seen[sy][sx] = true;
            bool portalReached = false;
            while (head < tail)
            {
                int16_t x = queue[head] % MAP_W, y = queue[head] / MAP_W;
                head++;
                const int8_t D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (auto &d : D)
                {
                    int16_t nx = x + d[0], ny = y + d[1];
                    if (nx == map.portalX && ny == map.portalY)
                    {
                        portalReached = true;
                    }
                    if (!map.solid(nx, ny) && !seen[ny][nx])
                    {
                        seen[ny][nx] = true;
                        queue[tail++] = (uint16_t)(ny * MAP_W + nx);
                    }
                }
            }
            CHECK(portalReached);
            for (uint8_t y = 0; y < MAP_H; y++)
            {
                for (uint8_t x = 0; x < MAP_W; x++)
                {
                    CHECK(map.solid(x, y) || seen[y][x]);
                }
            }
            for (uint8_t i = 0; i < Run::MAX_ENEMIES; i++)
            {
                const Enemy &e = run.getEnemies()[i];
                if (e.state != EnemyState::DEAD)
                {
                    CHECK(!map.solidAt(e.x, e.y));
                }
            }
            for (uint8_t i = 0; i < Run::MAX_PICKUPS; i++)
            {
                const Pickup &p = run.getPickups()[i];
                if (p.active)
                {
                    CHECK(!map.solidAt(p.x, p.y));
                }
            }
            run.nextFloor();
        }
        if (failures > 20)
        {
            return;
        }
    }

    Run a, b;
    a.newRun(77);
    b.newRun(77);
    CHECK(memcmp(&a.getMap().cells, &b.getMap().cells, sizeof(a.getMap().cells)) == 0);
}

static Run emptyRoom()
{
    Run run;
    run.newRun(5);
    for (uint8_t i = 0; i < Run::MAX_ENEMIES; i++)
    {
        run.enemyAt(i)->state = EnemyState::DEAD;
    }
    const Room &r = run.getMap().rooms[0];
    run.place(r.centerX() + 0.5f, r.centerY() + 0.5f, 0);
    return run;
}

static void testCollision()
{
    Run run = emptyRoom();
    Input fwd = {};
    fwd.forward = true;
    for (int a = 0; a < 8; a++)
    {
        run.place(run.getMap().rooms[0].centerX() + 0.5f, run.getMap().rooms[0].centerY() + 0.5f, a * 0.785f);
        step(run, fwd, 6000);
        CHECK(!run.getMap().solidAt(run.getX(), run.getY()));
        CHECK(!run.getMap().solidAt(run.getX() + PLAYER_RADIUS * 0.9f, run.getY()));
        CHECK(!run.getMap().solidAt(run.getX(), run.getY() - PLAYER_RADIUS * 0.9f));
    }
}

static void testSwing()
{
    Run run = emptyRoom();
    float x = run.getX(), y = run.getY();
    Enemy *ahead = run.spawnEnemy(ENEMY_ZOMBIE, x + 1.0f, y);
    Enemy *behind = run.spawnEnemy(ENEMY_ZOMBIE, x - 1.0f, y);
    Enemy *far = run.spawnEnemy(ENEMY_ZOMBIE, x + 2.5f, y);
    int16_t full = ahead->hp;
    Input tap = {};
    tap.attack = true;
    run.update(tap, 20);
    run.update(IDLE_INPUT, 20);
    uint32_t ev = run.takeEvents();
    CHECK(ev & EV_SWING);
    CHECK(ahead->hp < full);
    CHECK(behind->hp == full);
    CHECK(far->hp == full || run.getMap().solidAt(x + 2.5f, y));
}

static void testCharge()
{
    Run run = emptyRoom();
    Input hold = {};
    hold.attack = true;
    uint16_t mana = run.getMana();
    step(run, hold, 100);
    run.update(IDLE_INPUT, 20);
    CHECK(run.takeEvents() & EV_SWING);
    CHECK(run.getMana() >= mana - 1);

    step(run, IDLE_INPUT, 500);
    mana = run.getMana();
    step(run, hold, 600);
    CHECK(run.isCharging());
    CHECK(run.getCharge() > 0 && run.getCharge() < 255);
    run.update(IDLE_INPUT, 20);
    uint32_t ev = run.takeEvents();
    CHECK(ev & EV_CAST);
    CHECK(!(ev & EV_SWING));
    CHECK(run.getMana() + SPELL_COST - 1 <= mana);
    bool flying = false;
    for (uint8_t i = 0; i < Run::MAX_PROJECTILES; i++)
    {
        flying |= run.getProjectiles()[i].active && !run.getProjectiles()[i].hostile;
    }
    CHECK(flying);

    // Out of mana nothing leaves the hand
    for (int i = 0; i < 10; i++)
    {
        step(run, hold, 400);
        run.update(IDLE_INPUT, 20);
    }
    run.takeEvents();
    step(run, hold, 400);
    run.update(IDLE_INPUT, 20);
    CHECK(run.takeEvents() & EV_NO_MANA);

    // A full charge hits harder than a half one
    Run a = emptyRoom(), b = emptyRoom();
    Enemy *ea = a.spawnEnemy(ENEMY_LICH, a.getX() + 2.0f, a.getY());
    Enemy *eb = b.spawnEnemy(ENEMY_LICH, b.getX() + 2.0f, b.getY());
    step(a, hold, CHARGE_MS + 100);
    step(b, hold, TAP_MS + 200);
    step(a, IDLE_INPUT, 600);
    step(b, IDLE_INPUT, 600);
    CHECK(ea->maxHp - ea->hp > eb->maxHp - eb->hp);
    CHECK(eb->hp < eb->maxHp);
}

static void testInventory()
{
    Inventory inv;
    inv.clear();
    Item displaced;
    Item weak = {SLOT_WEAPON, 0, 0, 0, 5};
    Item strong = {SLOT_WEAPON, 1, 1, 0, 12};
    CHECK(inv.pickUp(weak, displaced) == Pick::EQUIPPED);
    CHECK(inv.pickUp(strong, displaced) == Pick::EQUIPPED);
    CHECK(inv.equipped[SLOT_WEAPON].value == 12);
    CHECK(inv.bagCount() == 1 && inv.bag[0].value == 5);
    Item worse = {SLOT_WEAPON, 0, 2, 0, 8};
    CHECK(inv.pickUp(worse, displaced) == Pick::BAGGED);
    CHECK(inv.equipped[SLOT_WEAPON].value == 12);
    inv.equipFromBag(1);
    CHECK(inv.equipped[SLOT_WEAPON].value == 8);
    CHECK(inv.bag[1].value == 12);

    for (uint8_t i = 0; i < BAG_SIZE; i++)
    {
        inv.bag[i] = Item{SLOT_RING, 0, 0, 0, 1};
    }
    Item ring = {SLOT_RING, 0, 0, 0, 1};
    CHECK(inv.pickUp(ring, displaced) == Pick::EQUIPPED);
    CHECK(displaced.empty());
    CHECK(inv.pickUp(ring, displaced) == Pick::BAG_FULL);
    CHECK(displaced.value == 1);
    Item better = {SLOT_RING, 2, 0, 0, 30};
    CHECK(inv.pickUp(better, displaced) == Pick::EQUIPPED);
    CHECK(displaced.value == 1); // The old ring had nowhere to go

    // Walking over gear picks it up; a refused item waits until the player steps off
    Run run = emptyRoom();
    Item helm = {SLOT_HELM, 0, 0, 0, 20};
    int16_t hp = run.getHp();
    run.dropItem(run.getX(), run.getY(), helm);
    run.update(IDLE_INPUT, 20);
    CHECK(run.takeEvents() & EV_EQUIP);
    CHECK(run.maxHp() == PLAYER_HP + 20);
    CHECK(run.getHp() == hp + 20);
}

static void testEnemiesHurt()
{
    Run run = emptyRoom();
    run.spawnEnemy(ENEMY_GOBLIN, run.getX() + 0.7f, run.getY());
    int16_t hp = run.getHp();
    step(run, IDLE_INPUT, 3000);
    CHECK(run.getHp() < hp);

    Run shot = emptyRoom();
    const Room &r = shot.getMap().rooms[0];
    shot.spawnEnemy(ENEMY_FROG_WIZARD, r.x + 0.5f, r.y + 0.5f);
    shot.place(r.x + r.w - 0.5f, r.y + r.h - 0.5f, 0);
    hp = shot.getHp();
    step(shot, IDLE_INPUT, 6000);
    CHECK(shot.getHp() < hp || !shot.getMap().clearLine(r.x + 0.5f, r.y + 0.5f, shot.getX(), shot.getY()));
}

static void testSave()
{
    Run run;
    run.newRun(1234);
    run.nextFloor();
    run.inventory.hpPotions = 3;
    run.nextFloor();
    Records records = {};
    records.runs = 4;
    SaveBlob blob;
    captureSave(blob, run, records);
    SlotStore store("grimhold");
    CHECK(writeSave(store, blob));
    SaveBlob back;
    CHECK(readSave(store, back));
    CHECK(back.records.runs == 4);
    CHECK(back.run.floor == 3);
    Run again;
    again.restore(back.run);
    CHECK(again.getFloor() == 3);
    CHECK(again.inventory.hpPotions == 3);
    CHECK(memcmp(&again.getMap().cells, &run.getMap().cells, sizeof(run.getMap().cells)) == 0);

    CHECK(recordRun(records, run));
    CHECK(records.bestFloor == 3 && records.runs == 5);
}

static void testBossFloor()
{
    Run run;
    run.newRun(9);
    while (!run.isBossFloor())
    {
        run.nextFloor();
    }
    CHECK(run.isSealed());
    const Enemy *boss = run.getBoss();
    CHECK(boss != nullptr);
    if (boss != nullptr)
    {
        CHECK(boss->kind == ENEMY_LICH);
    }
}

// Each hint waits for its action, in order; leaving the start room ends it at once
static void testTutorial()
{
    Run run = emptyRoom();
    Tutorial t;
    t.start(run);
    Input input = {};
    auto tick = [&](const Input &in, int ms) {
        for (int i = 0; i < ms; i += 20)
        {
            run.update(in, 20);
            t.update(run, run.takeEvents(), 20);
        }
    };
    tick(input, 200);
    CHECK(t.getStep() == Tutorial::WALK);
    // Walks along the room's long side, so the step is taught before reaching a wall
    const Room &r = run.getMap().rooms[0];
    run.place(r.x + 0.5f, r.y + r.h / 2 + 0.5f, 0);
    t.start(run);
    input.forward = true;
    tick(input, 600);
    CHECK(t.getStep() == Tutorial::TURN);
    input = {};
    input.left = true;
    tick(input, 300);
    CHECK(t.getStep() == Tutorial::TURN);
    tick(input, 400);
    CHECK(t.getStep() == Tutorial::SWORD);
    input = {};
    input.attack = true;
    tick(input, 20);
    tick({}, 20);
    CHECK(t.getStep() == Tutorial::SPELL);
    tick(input, 600);
    CHECK(t.getStep() == Tutorial::SPELL);
    tick({}, 20);
    CHECK(t.getStep() == Tutorial::PAUSE);
    tick({}, Tutorial::PAUSE_HINT_MS + 40);
    CHECK(t.isDone());

    Run away = emptyRoom();
    Tutorial left;
    left.start(away);
    const Room &start = away.getMap().rooms[0];
    away.place(start.x + start.w + 3.5f, start.y + 0.5f, 0);
    CHECK(left.update(away, 0, 20));
    CHECK(left.isDone());
}

int main()
{
    testFloors();
    testCollision();
    testSwing();
    testCharge();
    testInventory();
    testEnemiesHurt();
    testSave();
    testBossFloor();
    testTutorial();
    if (failures == 0)
    {
        printf("All tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
