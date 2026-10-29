// Logic tests on the PC: moving, turning, walls, eating, speeding up, the food's spread and the
// saved records. Build and run with tools/test.sh.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
#include "../src/snake/Records.h"
#include "../src/snake/Snake.h"
#include <Preferences.h>

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0x12345678u;
static uint32_t xorshift()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static Snake snake;

static Cell headCell() { return snake.segment(0); }

static bool safe(Cell h, Dir d)
{
    int x = h.x + (d == Dir::RIGHT) - (d == Dir::LEFT), y = h.y + (d == Dir::DOWN) - (d == Dir::UP);
    if (x < 0 || x >= Snake::COLS || y < 0 || y >= Snake::ROWS)
    {
        return false;
    }
    return !snake.occupied({(uint8_t)x, (uint8_t)y});
}

// Steps until the head reaches the food, steering at it and around itself
static Snake::Result eatOne()
{
    for (int guard = 0; guard < 400; guard++)
    {
        Cell h = headCell(), f = snake.food();
        Dir wanted[4] = {h.x < f.x ? Dir::RIGHT : Dir::LEFT, h.y < f.y ? Dir::DOWN : Dir::UP,
                         h.y < f.y ? Dir::UP : Dir::DOWN, h.x < f.x ? Dir::LEFT : Dir::RIGHT};
        if (h.x == f.x)
        {
            std::swap(wanted[0], wanted[1]);
        }
        for (Dir d : wanted)
        {
            if (d != opposite(snake.heading()) && safe(h, d))
            {
                snake.turn(d);
                break;
            }
        }
        Snake::Result r = snake.step();
        if (r != Snake::MOVED)
        {
            return r;
        }
    }
    return Snake::DIED;
}

static void testStart()
{
    snake.reset(1, xorshift);
    assert(snake.length() == Snake::START_LENGTH);
    assert(snake.heading() == Dir::RIGHT);
    assert(snake.hasFood());
    assert(!snake.occupied(snake.food()));
    for (uint16_t i = 1; i < snake.length(); i++)
    {
        assert(snake.segment(i).x + i == headCell().x);
    }
    printf("  start ok\n");
}

static void testTurns()
{
    snake.reset(1, xorshift);
    Cell h = headCell();
    snake.turn(Dir::LEFT); // A reversal is dropped
    snake.step();
    assert(headCell().x == h.x + 1 && headCell().y == h.y);

    // Two presses in one step make a U-turn over two steps
    h = headCell();
    snake.turn(Dir::UP);
    snake.turn(Dir::LEFT);
    snake.step();
    assert(headCell().x == h.x && headCell().y == h.y - 1);
    snake.step();
    assert(headCell().x == h.x - 1 && headCell().y == h.y - 1);
    assert(snake.heading() == Dir::LEFT);
    printf("  turns ok\n");
}

static void testWalls()
{
    snake.reset(1, xorshift);
    snake.turn(Dir::UP);
    Snake::Result r = Snake::MOVED;
    for (int i = 0; i < Snake::ROWS + 1 && r != Snake::DIED; i++)
    {
        r = snake.step();
    }
    assert(r == Snake::DIED);
    assert(snake.length() >= Snake::START_LENGTH);

    snake.reset(0, xorshift);
    snake.turn(Dir::UP);
    uint8_t x = headCell().x;
    for (int i = 0; i < Snake::ROWS * 2; i++)
    {
        r = snake.step();
        assert(r != Snake::DIED);
    }
    assert(headCell().x == x);
    printf("  walls ok\n");
}

static void testEating()
{
    snake.reset(2, xorshift);
    uint16_t len = snake.length(), ms = snake.getStepMs();
    assert(eatOne() == Snake::ATE);
    assert(snake.length() == len + 1);
    assert(snake.getScore() == LEVELS[2].points);
    assert(snake.fat(0));
    assert(snake.getStepMs() == ms - LEVELS[2].speedUpMs);
    Cell freed;
    snake.step();
    assert(!snake.fat(0) && snake.fat(1));
    assert(snake.vacated(freed));

    for (int i = 0; i < 60; i++)
    {
        if (eatOne() != Snake::ATE)
        {
            break;
        }
    }
    assert(snake.getStepMs() >= LEVELS[2].fastestMs);
    printf("  eating ok: %u eaten, step %u ms\n", (unsigned)snake.getEaten(), (unsigned)snake.getStepMs());
}

// A ring of four cells: the head moves into the cell the tail leaves in the same step
static void testFollowTail()
{
    snake.reset(0, xorshift);
    // Shorten to four in a square: right, down, left, up, round and round
    const Dir loop[] = {Dir::DOWN, Dir::LEFT, Dir::UP, Dir::RIGHT};
    for (int i = 0; i < 40; i++)
    {
        if (snake.hasFood())
        {
            Cell a;
            snake.ahead(a);
            if (a == snake.food())
            {
                break;
            }
        }
        snake.turn(loop[i % 4]);
        Snake::Result r = snake.step();
        assert(r != Snake::DIED);
    }
    printf("  following the tail ok\n");
}

static void testBite()
{
    snake.reset(1, xorshift);
    for (int i = 0; i < 4; i++)
    {
        assert(eatOne() == Snake::ATE);
    }
    snake.turn(Dir::UP);
    snake.step();
    snake.turn(Dir::LEFT);
    snake.step();
    snake.turn(Dir::DOWN);
    Snake::Result r = snake.step();
    assert(r == Snake::DIED);
    printf("  biting itself ok\n");
}

static void testFoodSpread()
{
    static uint32_t counts[Snake::CELLS];
    memset(counts, 0, sizeof(counts));
    const uint32_t samples = 200000;
    uint32_t free = 0;
    for (uint32_t i = 0; i < samples; i++)
    {
        snake.reset(1, xorshift);
        Cell f = snake.food();
        counts[f.y * Snake::COLS + f.x]++;
    }
    double expected = 0, chi = 0;
    snake.reset(1, xorshift);
    for (uint16_t i = 0; i < Snake::CELLS; i++)
    {
        Cell c = {(uint8_t)(i % Snake::COLS), (uint8_t)(i / Snake::COLS)};
        if (snake.occupied(c))
        {
            assert(counts[i] == 0);
        }
        else
        {
            free++;
        }
    }
    expected = (double)samples / free;
    for (uint16_t i = 0; i < Snake::CELLS; i++)
    {
        Cell c = {(uint8_t)(i % Snake::COLS), (uint8_t)(i / Snake::COLS)};
        if (!snake.occupied(c))
        {
            double d = counts[i] - expected;
            chi += d * d / expected;
        }
    }
    double bound = free + 6 * sqrt(2.0 * free);
    printf("  food spread chi2 %.1f over %u cells (bound %.0f)\n", chi, (unsigned)free, bound);
    assert(chi < bound);
}

static void testRecords()
{
    SlotStore store("snake");
    Records r;
    defaultRecords(r);
    assert(!readRecords(store, r));
    r.level = 3;
    r.best[0] = 12;
    r.best[3] = 345;
    assert(writeRecords(store, r));
    Records back;
    defaultRecords(back);
    assert(readRecords(store, back));
    assert(back.level == 3 && back.best[0] == 12 && back.best[3] == 345);

    r.level = 9;
    assert(writeRecords(store, r));
    assert(readRecords(store, back));
    assert(back.level == 1);
    store.erase();
    assert(!readRecords(store, back));
    printf("  records ok\n");
}

int main()
{
    printf("Snake logic\n");
    testStart();
    testTurns();
    testWalls();
    testEating();
    testFollowTail();
    testBite();
    testFoodSpread();
    testRecords();
    printf("All passed\n");
    return 0;
}
