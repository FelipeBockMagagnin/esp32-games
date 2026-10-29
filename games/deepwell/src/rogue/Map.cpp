#include "Map.h"
#include <string.h>

static const uint8_t ROOM_ATTEMPTS = 80;
static const uint8_t MIN_ROOM = 3;
static const uint8_t MAX_ROOM_W = 6;
static const uint8_t MAX_ROOM_H = 5;

static bool overlaps(const Room &a, const Room &b)
{
    // A wall's width between rooms, so two never merge into one
    return a.x <= b.x + b.w && b.x <= a.x + a.w && a.y <= b.y + b.h && b.y <= a.y + a.h;
}

void Map::carve(int16_t x, int16_t y)
{
    if (x > 0 && y > 0 && x < MAP_W - 1 && y < MAP_H - 1)
    {
        tiles[y][x] = TILE_FLOOR;
    }
}

void Map::carveRoom(const Room &room)
{
    for (uint8_t y = room.y; y < room.y + room.h; y++)
    {
        for (uint8_t x = room.x; x < room.x + room.w; x++)
        {
            carve(x, y);
        }
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

void Map::generate(Rng &rng)
{
    memset(tiles, TILE_WALL, sizeof(tiles));
    roomCount = 0;

    for (uint8_t attempt = 0; attempt < ROOM_ATTEMPTS && roomCount < MAX_ROOMS; attempt++)
    {
        Room room;
        room.w = (uint8_t)rng.range(MIN_ROOM, MAX_ROOM_W);
        room.h = (uint8_t)rng.range(MIN_ROOM, MAX_ROOM_H);
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
        carveRoom(rooms[i]);
        if (i > 0)
        {
            carveCorridor(rooms[i - 1], rooms[i], rng.below(2) == 0);
        }
    }

    const Room &first = rooms[0];
    startX = first.centerX();
    startY = first.centerY();

    uint8_t far = 0;
    int16_t farDistance = -1;
    for (uint8_t i = 1; i < roomCount; i++)
    {
        int16_t dx = rooms[i].centerX() - startX, dy = rooms[i].centerY() - startY;
        int16_t distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (distance > farDistance)
        {
            farDistance = distance;
            far = i;
        }
    }
    if (far == 0)
    {
        tiles[first.y + first.h - 1][first.x + first.w - 1] = TILE_STAIRS;
    }
    else
    {
        tiles[rooms[far].centerY()][rooms[far].centerX()] = TILE_STAIRS;
    }
}
