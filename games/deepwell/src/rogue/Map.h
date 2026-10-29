#ifndef ROGUE_MAP_H
#define ROGUE_MAP_H

#include "Config.h"
#include "Rng.h"

enum Tile : uint8_t
{
    TILE_WALL,
    TILE_FLOOR,
    TILE_STAIRS
};

struct Room
{
    uint8_t x, y, w, h;

    uint8_t centerX() const { return x + w / 2; }
    uint8_t centerY() const { return y + h / 2; }
};

struct Map
{
    static const uint8_t MAX_ROOMS = 6;

    uint8_t tiles[MAP_H][MAP_W];
    Room rooms[MAX_ROOMS];
    uint8_t roomCount;
    uint8_t startX, startY;

    // Rooms joined in order by L-shaped corridors, so every room reaches every other. The
    // player starts in the first room and the stairs go in the one farthest from it.
    void generate(Rng &rng);

    Tile at(int16_t x, int16_t y) const
    {
        return x < 0 || y < 0 || x >= MAP_W || y >= MAP_H ? TILE_WALL : (Tile)tiles[y][x];
    }
    bool walkable(int16_t x, int16_t y) const { return at(x, y) != TILE_WALL; }

private:
    void carve(int16_t x, int16_t y);
    void carveRoom(const Room &room);
    void carveCorridor(const Room &from, const Room &to, bool horizontalFirst);
};

#endif // ROGUE_MAP_H
