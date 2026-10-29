#ifndef MINES_FIELD_H
#define MINES_FIELD_H

#include <stdint.h>

using RandomSource = uint32_t (*)();

struct Cell
{
    uint8_t x;
    uint8_t y;

    bool operator==(const Cell &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Cell &o) const { return !(*this == o); }
};

struct Level
{
    const char *name; // 7 chars max
    uint8_t cols;
    uint8_t rows;
    uint8_t mines;
    uint8_t tile; // Pixels per side, so every level fills the screen below the HUD
};

static const uint8_t LEVEL_COUNT = 3;
extern const Level LEVELS[LEVEL_COUNT];

// The rules on a grid, with no drawing. Every open or chord lists the cells it opened in
// flood order, each with its wave (its distance from where the flood began), so the screen
// can reveal them ring by ring.
class Field
{
public:
    static const uint8_t MAX_COLS = 14;
    static const uint8_t MAX_ROWS = 18;
    static const uint16_t MAX_CELLS = MAX_COLS * MAX_ROWS;

    enum Result : uint8_t
    {
        NOTHING,
        OPENED,
        HIT,
        WON
    };

    struct Opened
    {
        Cell cell;
        uint8_t wave;
    };

    // The mines are laid on the first open, never on that cell or its neighbours
    void reset(uint8_t cols, uint8_t rows, uint8_t mines, RandomSource random);

    Result open(Cell c);
    // On an open number whose flags match it, opens every covered neighbour that has no flag
    Result chord(Cell c);
    // False when the cell is open
    bool toggleFlag(Cell c);

    uint8_t cols() const { return w; }
    uint8_t rows() const { return h; }
    uint8_t mineCount() const { return mines; }
    bool armed() const { return laid; }
    bool over() const { return lost || won; }
    bool isWon() const { return won; }
    Cell hitCell() const { return hit; }

    bool isMine(Cell c) const { return at(c) & MINE; }
    bool isOpen(Cell c) const { return at(c) & OPEN; }
    bool isFlagged(Cell c) const { return at(c) & FLAG; }
    uint8_t count(Cell c) const { return at(c) & COUNT; }
    int16_t minesLeft() const { return (int16_t)mines - flags; }
    uint16_t safeLeft() const { return safeCovered; }

    uint16_t openedCount() const { return listLength; }
    const Opened &opened(uint16_t i) const { return list[i]; }

    // Writes the neighbours of c that lie inside the field (8 at most) and returns how many
    uint8_t neighbours(Cell c, Cell *out) const;

private:
    static const uint8_t COUNT = 0x0F;
    static const uint8_t MINE = 0x10;
    static const uint8_t OPEN = 0x20;
    static const uint8_t FLAG = 0x40;

    uint8_t cells[MAX_CELLS];
    Opened list[MAX_CELLS];
    uint16_t listLength = 0;
    uint8_t w = 0, h = 0, mines = 0;
    int16_t flags = 0;
    uint16_t safeCovered = 0;
    bool laid = false, lost = false, won = false;
    Cell hit = {0, 0};
    RandomSource random = nullptr;

    uint8_t &at(Cell c) { return cells[c.y * w + c.x]; }
    uint8_t at(Cell c) const { return cells[c.y * w + c.x]; }
    void lay(Cell safe);
    bool push(Cell c, uint8_t wave);
    Result flood(uint16_t from);
    Result settle();
};

#endif // MINES_FIELD_H
