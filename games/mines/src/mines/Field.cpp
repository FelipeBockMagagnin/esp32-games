#include "Field.h"
#include <string.h>

const Level LEVELS[LEVEL_COUNT] = {
    {"Easy", 9, 12, 10, 24},
    {"Normal", 12, 16, 30, 18},
    {"Hard", 14, 18, 50, 16},
};

static uint32_t uniform(RandomSource random, uint32_t n)
{
    return (uint32_t)(((uint64_t)random() * n) >> 32);
}

void Field::reset(uint8_t cols, uint8_t rows, uint8_t mineCount, RandomSource source)
{
    w = cols > MAX_COLS ? MAX_COLS : cols;
    h = rows > MAX_ROWS ? MAX_ROWS : rows;
    uint16_t most = w * h > 9 ? w * h - 9 : 0;
    mines = mineCount > most ? (uint8_t)most : mineCount;
    random = source;
    memset(cells, 0, sizeof(cells));
    listLength = 0;
    flags = 0;
    safeCovered = w * h - mines;
    laid = lost = won = false;
    hit = {0, 0};
}

uint8_t Field::neighbours(Cell c, Cell *out) const
{
    uint8_t n = 0;
    for (int8_t dy = -1; dy <= 1; dy++)
    {
        for (int8_t dx = -1; dx <= 1; dx++)
        {
            int16_t x = c.x + dx, y = c.y + dy;
            if ((dx != 0 || dy != 0) && x >= 0 && x < w && y >= 0 && y < h)
            {
                out[n++] = {(uint8_t)x, (uint8_t)y};
            }
        }
    }
    return n;
}

void Field::lay(Cell safe)
{
    uint16_t free = 0;
    for (uint8_t y = 0; y < h; y++)
    {
        for (uint8_t x = 0; x < w; x++)
        {
            bool near = (x + 1 >= safe.x && x <= safe.x + 1) && (y + 1 >= safe.y && y <= safe.y + 1);
            free += !near;
        }
    }
    for (uint8_t m = 0; m < mines; m++, free--)
    {
        uint32_t k = uniform(random, free);
        for (uint8_t y = 0; y < h; y++)
        {
            for (uint8_t x = 0; x < w; x++)
            {
                Cell c = {x, y};
                bool near = (x + 1 >= safe.x && x <= safe.x + 1) && (y + 1 >= safe.y && y <= safe.y + 1);
                if (near || (at(c) & MINE))
                {
                    continue;
                }
                if (k-- == 0)
                {
                    at(c) |= MINE;
                    Cell around[8];
                    uint8_t n = neighbours(c, around);
                    for (uint8_t i = 0; i < n; i++)
                    {
                        at(around[i])++;
                    }
                    goto placed;
                }
            }
        }
    placed:;
    }
    laid = true;
}

bool Field::push(Cell c, uint8_t wave)
{
    uint8_t &v = at(c);
    if (v & (OPEN | FLAG))
    {
        return false;
    }
    v |= OPEN;
    list[listLength++] = {c, wave};
    if (v & MINE)
    {
        if (!lost)
        {
            hit = c;
        }
        lost = true;
    }
    else
    {
        safeCovered--;
    }
    return true;
}

// Spreads from every listed cell from `from` on: a cell with no mine around opens its neighbours
Field::Result Field::flood(uint16_t from)
{
    for (uint16_t i = from; i < listLength; i++)
    {
        Opened o = list[i];
        if (at(o.cell) & (MINE | COUNT))
        {
            continue;
        }
        Cell around[8];
        uint8_t n = neighbours(o.cell, around);
        for (uint8_t k = 0; k < n; k++)
        {
            push(around[k], o.wave + 1);
        }
    }
    return settle();
}

Field::Result Field::settle()
{
    if (lost)
    {
        return HIT;
    }
    if (safeCovered == 0)
    {
        won = true;
        for (uint16_t i = 0; i < (uint16_t)(w * h); i++)
        {
            if ((cells[i] & MINE) && !(cells[i] & FLAG))
            {
                cells[i] |= FLAG;
                flags++;
            }
        }
        return WON;
    }
    return listLength > 0 ? OPENED : NOTHING;
}

Field::Result Field::open(Cell c)
{
    listLength = 0;
    if (over() || c.x >= w || c.y >= h)
    {
        return NOTHING;
    }
    if (!laid)
    {
        lay(c);
    }
    if (!push(c, 0))
    {
        return NOTHING;
    }
    return flood(0);
}

Field::Result Field::chord(Cell c)
{
    listLength = 0;
    if (over() || c.x >= w || c.y >= h || !(at(c) & OPEN) || count(c) == 0)
    {
        return NOTHING;
    }
    Cell around[8];
    uint8_t n = neighbours(c, around);
    uint8_t flagged = 0;
    for (uint8_t i = 0; i < n; i++)
    {
        flagged += isFlagged(around[i]);
    }
    if (flagged != count(c))
    {
        return NOTHING;
    }
    for (uint8_t i = 0; i < n; i++)
    {
        push(around[i], 0);
    }
    return flood(0);
}

bool Field::toggleFlag(Cell c)
{
    if (over() || c.x >= w || c.y >= h || (at(c) & OPEN))
    {
        return false;
    }
    at(c) ^= FLAG;
    flags += (at(c) & FLAG) ? 1 : -1;
    return true;
}
