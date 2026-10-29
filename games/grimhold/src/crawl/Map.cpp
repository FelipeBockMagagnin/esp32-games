#include "Map.h"
#include <math.h>
#include <string.h>

static const uint8_t ROOM_ATTEMPTS = 120;
static const uint8_t MIN_ROOM = 3;
static const uint8_t MAX_ROOM = 7;
static const uint8_t PILLAR_ROOM = 5;
static const uint8_t ACCENT_ONE_IN = 7;

static bool overlaps(const Room &a, const Room &b)
{
    // A wall's width between rooms, so two never merge into one
    return a.x <= b.x + b.w && b.x <= a.x + a.w && a.y <= b.y + b.h && b.y <= a.y + a.h;
}

uint8_t wallSetFor(uint8_t floor)
{
    uint8_t i = 0;
    while (i + 1 < WALL_SET_COUNT && floor >= WALL_SETS[i + 1].firstFloor)
    {
        i++;
    }
    return i;
}

void Map::carve(int16_t x, int16_t y)
{
    if (x > 0 && y > 0 && x < MAP_W - 1 && y < MAP_H - 1)
    {
        cells[y][x] = CELL_FLOOR;
    }
}

void Map::carveCorridor(const Room &from, const Room &to, bool horizontalFirst)
{
    int16_t x = from.centerX(), y = from.centerY();
    int16_t tx = to.centerX(), ty = to.centerY();
    for (uint8_t leg = 0; leg < 2; leg++)
    {
        if (horizontalFirst == (leg == 0))
        {
            while (x != tx)
            {
                x += x < tx ? 1 : -1;
                carve(x, y);
            }
        }
        else
        {
            while (y != ty)
            {
                y += y < ty ? 1 : -1;
                carve(x, y);
            }
        }
    }
}

void Map::generate(Rng &rng, uint8_t floor)
{
    const WallSet &set = WALL_SETS[wallSetFor(floor)];
    for (uint8_t y = 0; y < MAP_H; y++)
    {
        for (uint8_t x = 0; x < MAP_W; x++)
        {
            cells[y][x] = rng.below(ACCENT_ONE_IN) == 0 ? set.accent : set.main;
        }
    }
    roomCount = 0;

    for (uint8_t attempt = 0; attempt < ROOM_ATTEMPTS && roomCount < MAX_ROOMS; attempt++)
    {
        Room room;
        room.w = (uint8_t)rng.range(MIN_ROOM, MAX_ROOM);
        room.h = (uint8_t)rng.range(MIN_ROOM, MAX_ROOM);
        room.x = (uint8_t)rng.range(1, MAP_W - 1 - room.w);
        room.y = (uint8_t)rng.range(1, MAP_H - 1 - room.h);
        bool clear = true;
        for (uint8_t i = 0; i < roomCount && clear; i++)
        {
            clear = !overlaps(room, rooms[i]);
        }
        if (clear)
        {
            rooms[roomCount++] = room;
        }
    }

    for (uint8_t i = 0; i < roomCount; i++)
    {
        const Room &r = rooms[i];
        for (uint8_t y = r.y; y < r.y + r.h; y++)
        {
            for (uint8_t x = r.x; x < r.x + r.w; x++)
            {
                carve(x, y);
            }
        }
        if (r.w >= PILLAR_ROOM && r.h >= PILLAR_ROOM && rng.below(2) == 0)
        {
            cells[r.y + 1][r.x + 1] = set.accent;
            cells[r.y + 1][r.x + r.w - 2] = set.accent;
            cells[r.y + r.h - 2][r.x + 1] = set.accent;
            cells[r.y + r.h - 2][r.x + r.w - 2] = set.accent;
        }
    }
    for (uint8_t i = 1; i < roomCount; i++)
    {
        carveCorridor(rooms[i - 1], rooms[i], rng.below(2) == 0);
    }

    const Room &first = rooms[0];
    portalRoom = 0;
    int16_t farDistance = -1;
    for (uint8_t i = 1; i < roomCount; i++)
    {
        int16_t dx = rooms[i].centerX() - first.centerX(), dy = rooms[i].centerY() - first.centerY();
        int16_t distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (distance > farDistance)
        {
            farDistance = distance;
            portalRoom = i;
        }
    }
    if (portalRoom == 0)
    {
        portalX = first.x + first.w - 1;
        portalY = first.y + first.h - 1;
    }
    else
    {
        portalX = rooms[portalRoom].centerX();
        portalY = rooms[portalRoom].centerY();
    }
    cells[portalY][portalX] = CELL_PORTAL;
}

bool Map::clearLine(float ax, float ay, float bx, float by) const
{
    int16_t x = (int16_t)ax, y = (int16_t)ay;
    int16_t tx = (int16_t)bx, ty = (int16_t)by;
    float dx = bx - ax, dy = by - ay;
    int8_t stepX = dx < 0 ? -1 : 1, stepY = dy < 0 ? -1 : 1;
    float deltaX = dx == 0 ? 1e30f : fabsf(1.0f / dx);
    float deltaY = dy == 0 ? 1e30f : fabsf(1.0f / dy);
    float sideX = dx < 0 ? (ax - x) * deltaX : (x + 1.0f - ax) * deltaX;
    float sideY = dy < 0 ? (ay - y) * deltaY : (y + 1.0f - ay) * deltaY;
    for (uint8_t guard = 0; guard < MAP_W + MAP_H; guard++)
    {
        if (x == tx && y == ty)
        {
            return true;
        }
        if (sideX < sideY)
        {
            sideX += deltaX;
            x += stepX;
        }
        else
        {
            sideY += deltaY;
            y += stepY;
        }
        if (solid(x, y))
        {
            return false;
        }
    }
    return true;
}
