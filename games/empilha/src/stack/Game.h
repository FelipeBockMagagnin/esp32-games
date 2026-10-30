#ifndef STACK_GAME_H
#define STACK_GAME_H

#include <stdint.h>

typedef uint32_t (*RandomSource)();

static const uint8_t COLS = 11;
static const uint8_t VISIBLE_ROWS = 21;
// Pieces are born here, their lowest row just showing at the top of the well
static const uint8_t HIDDEN_ROWS = 2;
static const uint8_t ROWS = VISIBLE_ROWS + HIDDEN_ROWS;

static const uint8_t PIECE_COUNT = 7;
static const uint8_t PIECE_CELLS = 4;
static const uint8_t NEXT_COUNT = 3;

static const uint8_t MAX_LEVEL = 15;
static const uint8_t ROWS_PER_LEVEL = 10;
static const uint8_t SPRINT_ROWS = 30;
static const uint8_t SPRINT_LEVEL = 5;
// Five digits fit the side panel at text size 2
static const uint32_t SCORE_CAP = 99999;

enum class Mode : uint8_t
{
    ENDLESS,
    SPRINT, // Thirty rows against the clock, at one speed
    CASCADE, // Endless, but what a clear leaves loose falls in clumps and can clear again
    COUNT
};
static const uint8_t MODE_COUNT = (uint8_t)Mode::COUNT;
static const char *const MODE_NAMES[MODE_COUNT] = {"Endless", "Sprint", "Cascade"};

struct Cell
{
    int8_t x;
    int8_t y;
};

struct Piece
{
    uint8_t type;
    uint8_t rot;
    int8_t x;
    int8_t y;
};

// The well and the falling piece, with no clock of its own: the screen hands it elapsed time
// through advance() and paces the clear and the settling, which are its animations, calling
// clearRows() and settleStep() when each is due.
class Game
{
public:
    enum class Phase : uint8_t
    {
        DROPPING,
        CLEARING, // fullRows() are waiting for clearRows()
        SETTLING, // Cascade only: clumps fall a row per settleStep()
        TOPPED,   // No room for the next piece
        FINISHED  // Sprint done
    };

    // takeEvents() flags
    static const uint8_t EVENT_LANDED = 1;
    static const uint8_t EVENT_ROWS = 2;
    static const uint8_t EVENT_LEVEL = 4;
    static const uint8_t EVENT_SPAWNED = 8;

    void reset(Mode mode, RandomSource random);

    bool move(int8_t dx);
    bool rotate();
    void hardDrop();
    void advance(uint16_t ms, bool soft);

    void clearRows();
    bool settleStep();

    static void cellsOf(uint8_t type, uint8_t rot, Cell out[PIECE_CELLS]);
    // The boxes pieces turn in are this wide; the side panel centres its previews with it
    static uint8_t boxSize(uint8_t type);

    Phase phase() const { return state; }
    Mode mode() const { return gameMode; }
    bool falling() const { return state == Phase::DROPPING; }
    bool over() const { return state == Phase::TOPPED || state == Phase::FINISHED; }
    // 0 empty, otherwise the piece type + 1
    uint8_t cell(uint8_t x, uint8_t y) const { return well[y][x]; }
    const Piece &piece() const { return current; }
    int8_t ghostY() const;
    uint8_t next(uint8_t i) const { return queue[i]; }
    uint32_t fullRows() const { return full; }
    uint8_t chain() const { return chainLength; }
    uint32_t score() const { return points; }
    uint8_t level() const { return currentLevel; }
    uint16_t rows() const { return rowsCleared; }
    uint16_t pieces() const { return piecesLanded; }
    uint32_t tenths() const { return elapsedMs / 100; }
    uint8_t takeEvents();

    // For the tests and the scripted scenes
    void setCell(uint8_t x, uint8_t y, uint8_t value) { well[y][x] = value; }
    void setPiece(const Piece &piece) { current = piece; }
    bool fits(const Piece &piece) const;

private:
    uint8_t well[ROWS][COLS];
    Piece current;
    uint8_t queue[NEXT_COUNT];
    uint8_t bag[PIECE_COUNT];
    uint8_t bagLeft = 0;
    RandomSource random = nullptr;

    Phase state = Phase::DROPPING;
    Mode gameMode = Mode::ENDLESS;
    uint32_t full = 0;
    uint8_t chainLength = 0;
    uint32_t points = 0;
    uint8_t currentLevel = 1;
    uint16_t rowsCleared = 0;
    uint16_t piecesLanded = 0;
    uint32_t elapsedMs = 0;
    uint8_t events = 0;

    uint16_t fallMs = 0;
    uint16_t lockMs = 0;
    uint8_t lockResets = 0;
    int8_t lowestY = 0;

    uint8_t pull();
    void spawn();
    bool grounded() const;
    bool stepDown();
    void land();
    void checkRows();
    void award(uint8_t count);
    uint8_t labelClumps(uint8_t labels[ROWS][COLS]) const;
};

#endif // STACK_GAME_H
