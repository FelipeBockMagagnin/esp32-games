#ifndef SNAKE_SNAKE_H
#define SNAKE_SNAKE_H

#include <stdint.h>

using RandomSource = uint32_t (*)();

enum class Dir : uint8_t
{
    UP,
    RIGHT,
    DOWN,
    LEFT
};

inline Dir opposite(Dir d)
{
    return (Dir)(((uint8_t)d + 2) & 3);
}

struct Cell
{
    uint8_t x;
    uint8_t y;

    bool operator==(const Cell &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Cell &o) const { return !(*this == o); }
};

struct Level
{
    const char *name;  // 7 chars max
    uint16_t startMs;  // Time per step
    uint16_t fastestMs;
    uint8_t speedUpMs; // Taken off the step per food
    uint8_t points;    // Per food
    bool wrap;         // The walls lead to the other side instead of killing
};

static const uint8_t LEVEL_COUNT = 4;
extern const Level LEVELS[LEVEL_COUNT];

// The rules on a grid, with no drawing: the renderer asks which cells changed after each step.
// Segment 0 is the head. Each segment keeps the direction the snake moved to enter its cell,
// which is all the renderer needs to join it to its neighbours.
class Snake
{
public:
    static const uint8_t COLS = 22;
    static const uint8_t ROWS = 28;
    static const uint16_t CELLS = COLS * ROWS;
    static const uint8_t START_LENGTH = 4;

    enum Result : uint8_t
    {
        MOVED,
        ATE,
        DIED,
        WON // No empty cell left for the food
    };

    void reset(uint8_t level, RandomSource random);

    // Queued, so two quick presses make a U-turn across two steps; a reversal or a repeat of
    // the direction already queued is dropped
    void turn(Dir d);
    Result step();

    uint16_t length() const { return len; }
    Cell segment(uint16_t i) const { return segs[at(i)].cell; }
    Dir entered(uint16_t i) const { return segs[at(i)].entered; }
    // Swallowed food: the segment where the head ate stays fat until the tail passes it
    bool fat(uint16_t i) const { return segs[at(i)].fat; }
    Dir heading() const { return segs[at(0)].entered; }

    Cell food() const { return foodCell; }
    bool hasFood() const { return foodPlaced; }
    // The cell the head would move into next, false when that is a wall
    bool ahead(Cell &out) const;
    // The cell the last step freed, when the tail moved
    bool vacated(Cell &out) const;
    bool occupied(Cell c) const { return grid[c.y * COLS + c.x] != 0; }

    uint8_t getLevel() const { return level; }
    uint16_t getScore() const { return score; }
    uint16_t getStepMs() const { return stepMs; }
    uint16_t getEaten() const { return eaten; }

private:
    struct Segment
    {
        Cell cell;
        Dir entered;
        bool fat;
    };

    Segment segs[CELLS];
    uint8_t grid[CELLS];
    uint16_t head = 0; // Index of segment 0 in segs; the ring grows backwards
    uint16_t len = 0;
    Dir queued[2];
    uint8_t queuedCount = 0;
    Cell foodCell = {0, 0};
    bool foodPlaced = false;
    Cell freed = {0, 0};
    bool freedValid = false;
    uint8_t level = 0;
    uint16_t score = 0;
    uint16_t stepMs = 0;
    uint16_t eaten = 0;
    RandomSource random = nullptr;

    uint16_t at(uint16_t i) const { return (uint16_t)((head + i) % CELLS); }
    void mark(Cell c, uint8_t v) { grid[c.y * COLS + c.x] = v; }
    bool next(Cell from, Dir d, Cell &out) const;
    bool placeFood();
};

#endif // SNAKE_SNAKE_H
