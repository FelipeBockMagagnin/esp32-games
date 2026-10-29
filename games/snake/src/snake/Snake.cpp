#include "Snake.h"
#include <string.h>

const Level LEVELS[LEVEL_COUNT] = {
    {"Easy", 200, 200, 0, 1, true},
    {"Normal", 140, 140, 0, 2, false},
    {"Hard", 120, 70, 2, 3, false},
    {"Insane", 75, 45, 1, 5, false},
};

// [0, n) with every value equally likely: the 2^32 % n raw values that would favour the low
// ones are drawn again
static uint32_t uniform(RandomSource source, uint32_t n)
{
    if (n <= 1)
    {
        return 0;
    }
    uint32_t threshold = (0u - n) % n;
    uint32_t r;
    do
    {
        r = source();
    } while (r < threshold);
    return r % n;
}

void Snake::reset(uint8_t newLevel, RandomSource source)
{
    level = newLevel < LEVEL_COUNT ? newLevel : 0;
    random = source;
    memset(grid, 0, sizeof(grid));
    len = 0;
    head = 0;
    queuedCount = 0;
    score = 0;
    eaten = 0;
    stepMs = LEVELS[level].startMs;
    freedValid = false;

    Cell start = {(uint8_t)(COLS / 2 - 3), (uint8_t)(ROWS / 2)};
    for (uint8_t i = 0; i < START_LENGTH; i++)
    {
        Cell c = {(uint8_t)(start.x - i), start.y};
        segs[i] = {c, Dir::RIGHT, false};
        mark(c, 1);
        len++;
    }
    placeFood();
}

void Snake::turn(Dir d)
{
    Dir last = queuedCount > 0 ? queued[queuedCount - 1] : heading();
    if (d == last || d == opposite(last) || queuedCount >= 2)
    {
        return;
    }
    queued[queuedCount++] = d;
}

bool Snake::next(Cell from, Dir d, Cell &out) const
{
    int x = from.x, y = from.y;
    switch (d)
    {
    case Dir::UP:
        y--;
        break;
    case Dir::DOWN:
        y++;
        break;
    case Dir::LEFT:
        x--;
        break;
    case Dir::RIGHT:
        x++;
        break;
    }
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
    {
        if (!LEVELS[level].wrap)
        {
            return false;
        }
        x = (x + COLS) % COLS;
        y = (y + ROWS) % ROWS;
    }
    out = {(uint8_t)x, (uint8_t)y};
    return true;
}

bool Snake::ahead(Cell &out) const
{
    Dir d = queuedCount > 0 ? queued[0] : heading();
    return next(segment(0), d, out);
}

bool Snake::vacated(Cell &out) const
{
    out = freed;
    return freedValid;
}

Snake::Result Snake::step()
{
    Dir d = heading();
    if (queuedCount > 0)
    {
        d = queued[0];
        queued[0] = queued[1];
        queuedCount--;
    }

    freedValid = false;
    Cell target;
    if (!next(segment(0), d, target))
    {
        return DIED;
    }

    bool eats = foodPlaced && target == foodCell;
    if (!eats)
    {
        // The tail leaves its cell in the same step, so the head may follow right behind it
        Cell tail = segment(len - 1);
        mark(tail, 0);
        len--;
        freed = tail;
        freedValid = true;
    }
    if (occupied(target))
    {
        if (freedValid)
        {
            mark(freed, 1);
            len++;
            freedValid = false;
        }
        return DIED;
    }

    head = (uint16_t)((head + CELLS - 1) % CELLS);
    segs[head] = {target, d, eats};
    mark(target, 1);
    len++;

    if (!eats)
    {
        return MOVED;
    }
    const Level &l = LEVELS[level];
    score += l.points;
    eaten++;
    if (stepMs > l.fastestMs + l.speedUpMs)
    {
        stepMs -= l.speedUpMs;
    }
    else
    {
        stepMs = l.fastestMs;
    }
    return placeFood() ? ATE : WON;
}

// The k-th empty cell, so every free cell is as likely however long the snake grows
bool Snake::placeFood()
{
    foodPlaced = false;
    uint16_t free = CELLS - len;
    if (free == 0)
    {
        return false;
    }
    uint32_t k = uniform(random, free);
    for (uint16_t i = 0; i < CELLS; i++)
    {
        if (grid[i] != 0)
        {
            continue;
        }
        if (k-- == 0)
        {
            foodCell = {(uint8_t)(i % COLS), (uint8_t)(i / COLS)};
            foodPlaced = true;
            return true;
        }
    }
    return false;
}
