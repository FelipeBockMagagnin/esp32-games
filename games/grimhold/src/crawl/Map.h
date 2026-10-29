#ifndef CRAWL_MAP_H
#define CRAWL_MAP_H

#include "Config.h"
#include "Rng.h"

struct Room
{
    uint8_t x, y, w, h;

    uint8_t centerX() const { return x + w / 2; }
    uint8_t centerY() const { return y + h / 2; }
};

struct Map
{
    static const uint8_t MAX_ROOMS = 10;

    uint8_t cells[MAP_H][MAP_W];
    Room rooms[MAX_ROOMS];
    uint8_t roomCount;
    uint8_t portalX, portalY;
    uint8_t portalRoom;

    // Rooms joined in order by L-shaped corridors, so every room reaches every other; the big ones
    // get pillars at their inner corners, off the corridors' lines. The player starts in the first
    // room and the portal stands in the middle of the one farthest from it.
    void generate(Rng &rng, uint8_t floor);

    uint8_t at(int16_t x, int16_t y) const
    {
        return x < 0 || y < 0 || x >= MAP_W || y >= MAP_H ? (uint8_t)CELL_CRYPT : cells[y][x];
    }
    bool solid(int16_t x, int16_t y) const { return at(x, y) != CELL_FLOOR; }
    bool solidAt(float x, float y) const { return solid((int16_t)x, (int16_t)y); }
    // Nothing solid on the segment between the two points
    bool clearLine(float ax, float ay, float bx, float by) const;

private:
    void carve(int16_t x, int16_t y);
    void carveCorridor(const Room &from, const Room &to, bool horizontalFirst);
};

// Index into WALL_SETS
uint8_t wallSetFor(uint8_t floor);

#endif // CRAWL_MAP_H
