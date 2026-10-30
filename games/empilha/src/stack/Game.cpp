#include "Game.h"
#include <string.h>

// Milliseconds per row, by level
static const uint16_t FALL_MS[MAX_LEVEL] = {800, 650, 530, 430, 350, 280, 225, 180, 145, 115, 90, 70, 55, 45, 35};
static const uint16_t SOFT_MS = 30;
static const uint16_t LOCK_MS = 450;
static const uint16_t SOFT_LOCK_MS = 120;
// Sliding or turning on the ground restarts the lock, this many times per row reached
static const uint8_t MAX_LOCK_RESETS = 12;
static const uint8_t POINTS[PIECE_CELLS + 1] = {0, 1, 3, 5, 9};
// A chain can bring down more than four rows at once
static const uint8_t POINTS_PER_EXTRA_ROW = 3;

struct Shape
{
    uint8_t size;
    Cell cells[PIECE_CELLS];
};

static const Shape SHAPES[PIECE_COUNT] = {
    {4, {{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
    {2, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
    {3, {{1, 0}, {0, 1}, {1, 1}, {2, 1}}},
    {3, {{1, 0}, {2, 0}, {0, 1}, {1, 1}}},
    {3, {{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
    {3, {{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
    {3, {{2, 0}, {0, 1}, {1, 1}, {2, 1}}},
};
static const uint8_t SQUARE = 1;

static const Cell KICKS[] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {-2, 0}, {2, 0}};

void Game::cellsOf(uint8_t type, uint8_t rot, Cell out[PIECE_CELLS])
{
    const Shape &shape = SHAPES[type];
    for (uint8_t i = 0; i < PIECE_CELLS; i++)
    {
        int8_t x = shape.cells[i].x, y = shape.cells[i].y;
        for (uint8_t r = 0; r < (rot & 3); r++)
        {
            int8_t turned = (int8_t)(shape.size - 1 - y);
            y = x;
            x = turned;
        }
        out[i] = {x, y};
    }
}

uint8_t Game::boxSize(uint8_t type)
{
    return SHAPES[type].size;
}

void Game::reset(Mode mode, RandomSource source)
{
    memset(well, 0, sizeof(well));
    random = source;
    gameMode = mode;
    state = Phase::DROPPING;
    bagLeft = 0;
    full = 0;
    chainLength = 0;
    points = 0;
    currentLevel = mode == Mode::SPRINT ? SPRINT_LEVEL : 1;
    rowsCleared = 0;
    piecesLanded = 0;
    elapsedMs = 0;
    events = 0;
    for (uint8_t i = 0; i < NEXT_COUNT; i++)
    {
        queue[i] = pull();
    }
    spawn();
}

// Every piece once per seven, shuffled, so no long droughts
uint8_t Game::pull()
{
    if (bagLeft == 0)
    {
        for (uint8_t i = 0; i < PIECE_COUNT; i++)
        {
            bag[i] = i;
        }
        for (uint8_t i = PIECE_COUNT - 1; i > 0; i--)
        {
            uint8_t j = (uint8_t)(random() % (i + 1));
            uint8_t t = bag[i];
            bag[i] = bag[j];
            bag[j] = t;
        }
        bagLeft = PIECE_COUNT;
    }
    return bag[--bagLeft];
}

void Game::spawn()
{
    uint8_t type = queue[0];
    for (uint8_t i = 1; i < NEXT_COUNT; i++)
    {
        queue[i - 1] = queue[i];
    }
    queue[NEXT_COUNT - 1] = pull();
    current = {type, 0, (int8_t)((COLS - SHAPES[type].size) / 2), 1};
    lowestY = current.y;
    fallMs = 0;
    lockMs = 0;
    lockResets = 0;
    events |= EVENT_SPAWNED;
    if (!fits(current))
    {
        state = Phase::TOPPED;
        return;
    }
    state = Phase::DROPPING;
}

bool Game::fits(const Piece &piece) const
{
    Cell cells[PIECE_CELLS];
    cellsOf(piece.type, piece.rot, cells);
    for (const Cell &c : cells)
    {
        int8_t x = piece.x + c.x, y = piece.y + c.y;
        if (x < 0 || x >= COLS || y >= ROWS)
        {
            return false;
        }
        if (y >= 0 && well[y][x] != 0)
        {
            return false;
        }
    }
    return true;
}

bool Game::grounded() const
{
    Piece below = current;
    below.y++;
    return !fits(below);
}

int8_t Game::ghostY() const
{
    Piece below = current;
    while (fits(below))
    {
        below.y++;
    }
    return below.y - 1;
}

bool Game::move(int8_t dx)
{
    if (state != Phase::DROPPING)
    {
        return false;
    }
    bool wasGrounded = grounded();
    Piece moved = current;
    moved.x += dx;
    if (!fits(moved))
    {
        return false;
    }
    current = moved;
    if (wasGrounded)
    {
        lockMs = 0;
        lockResets++;
    }
    return true;
}

bool Game::rotate()
{
    if (state != Phase::DROPPING || current.type == SQUARE)
    {
        return false;
    }
    bool wasGrounded = grounded();
    Piece turned = current;
    turned.rot = (turned.rot + 1) & 3;
    for (const Cell &kick : KICKS)
    {
        Piece kicked = turned;
        kicked.x += kick.x;
        kicked.y += kick.y;
        if (fits(kicked))
        {
            current = kicked;
            if (wasGrounded)
            {
                lockMs = 0;
                lockResets++;
            }
            return true;
        }
    }
    return false;
}

bool Game::stepDown()
{
    if (grounded())
    {
        return false;
    }
    current.y++;
    if (current.y > lowestY)
    {
        lowestY = current.y;
        lockResets = 0;
        lockMs = 0;
    }
    return true;
}

void Game::hardDrop()
{
    if (state != Phase::DROPPING)
    {
        return;
    }
    current.y = ghostY();
    land();
}

void Game::advance(uint16_t ms, bool soft)
{
    if (over())
    {
        return;
    }
    elapsedMs += ms;
    if (state != Phase::DROPPING)
    {
        return;
    }
    uint16_t every = FALL_MS[currentLevel - 1];
    if (soft && SOFT_MS < every)
    {
        every = SOFT_MS;
    }
    fallMs += ms;
    while (fallMs >= every)
    {
        fallMs -= every;
        if (!stepDown())
        {
            fallMs = 0;
            break;
        }
    }
    if (!grounded())
    {
        lockMs = 0;
        return;
    }
    lockMs += ms;
    if (lockMs >= (soft ? SOFT_LOCK_MS : LOCK_MS) || lockResets >= MAX_LOCK_RESETS)
    {
        land();
    }
}

void Game::land()
{
    Cell cells[PIECE_CELLS];
    cellsOf(current.type, current.rot, cells);
    for (const Cell &c : cells)
    {
        int8_t y = current.y + c.y;
        if (y >= 0)
        {
            well[y][current.x + c.x] = current.type + 1;
        }
    }
    piecesLanded++;
    chainLength = 0;
    events |= EVENT_LANDED;
    checkRows();
}

void Game::checkRows()
{
    full = 0;
    uint8_t count = 0;
    for (uint8_t y = 0; y < ROWS; y++)
    {
        bool complete = true;
        for (uint8_t x = 0; x < COLS && complete; x++)
        {
            complete = well[y][x] != 0;
        }
        if (complete)
        {
            full |= 1UL << y;
            count++;
        }
    }
    if (count > 0)
    {
        chainLength++;
        award(count);
        state = Phase::CLEARING;
        events |= EVENT_ROWS;
        return;
    }
    if (gameMode == Mode::SPRINT && rowsCleared >= SPRINT_ROWS)
    {
        state = Phase::FINISHED;
        return;
    }
    spawn();
}

void Game::award(uint8_t count)
{
    uint32_t base = count <= PIECE_CELLS ? POINTS[count] : POINTS[PIECE_CELLS] + (count - PIECE_CELLS) * POINTS_PER_EXTRA_ROW;
    points += base * currentLevel * chainLength;
    if (points > SCORE_CAP)
    {
        points = SCORE_CAP;
    }
    rowsCleared += count;
    if (gameMode == Mode::SPRINT)
    {
        return;
    }
    uint16_t reached = 1 + rowsCleared / ROWS_PER_LEVEL;
    if (reached > MAX_LEVEL)
    {
        reached = MAX_LEVEL;
    }
    if (reached != currentLevel)
    {
        currentLevel = (uint8_t)reached;
        events |= EVENT_LEVEL;
    }
}

void Game::clearRows()
{
    if (state != Phase::CLEARING)
    {
        return;
    }
    if (gameMode == Mode::CASCADE)
    {
        for (uint8_t y = 0; y < ROWS; y++)
        {
            if (full & (1UL << y))
            {
                memset(well[y], 0, COLS);
            }
        }
        state = Phase::SETTLING;
        return;
    }
    int8_t to = ROWS - 1;
    for (int8_t y = ROWS - 1; y >= 0; y--)
    {
        if (!(full & (1UL << y)))
        {
            if (to != y)
            {
                memcpy(well[to], well[y], COLS);
            }
            to--;
        }
    }
    for (; to >= 0; to--)
    {
        memset(well[to], 0, COLS);
    }
    checkRows();
}

// Numbers each clump of touching cells from 1, and returns how many there are
uint8_t Game::labelClumps(uint8_t labels[ROWS][COLS]) const
{
    memset(labels, 0, ROWS * COLS);
    uint8_t count = 0;
    uint8_t stack[ROWS * COLS];
    for (uint8_t y = 0; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            if (well[y][x] == 0 || labels[y][x] != 0)
            {
                continue;
            }
            count++;
            labels[y][x] = count;
            uint8_t top = 0;
            stack[top++] = (uint8_t)(y * COLS + x);
            while (top > 0)
            {
                uint8_t at = stack[--top];
                uint8_t cx = at % COLS, cy = at / COLS;
                static const int8_t DX[4] = {1, -1, 0, 0};
                static const int8_t DY[4] = {0, 0, 1, -1};
                for (uint8_t d = 0; d < 4; d++)
                {
                    int8_t nx = cx + DX[d], ny = cy + DY[d];
                    if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS)
                    {
                        continue;
                    }
                    if (well[ny][nx] != 0 && labels[ny][nx] == 0)
                    {
                        labels[ny][nx] = count;
                        stack[top++] = (uint8_t)(ny * COLS + nx);
                    }
                }
            }
        }
    }
    return count;
}

// A clump rests on the floor or on a clump that rests. Every clump that does not drops one row
// together, so they keep their shape; false once none is left to fall.
bool Game::settleStep()
{
    if (state != Phase::SETTLING)
    {
        return false;
    }
    uint8_t labels[ROWS][COLS];
    uint8_t count = labelClumps(labels);
    bool rests[ROWS * COLS / 2 + 2] = {false};
    for (uint8_t x = 0; x < COLS; x++)
    {
        rests[labels[ROWS - 1][x]] = true;
    }
    rests[0] = false;
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int8_t y = ROWS - 2; y >= 0; y--)
        {
            for (uint8_t x = 0; x < COLS; x++)
            {
                uint8_t here = labels[y][x], under = labels[y + 1][x];
                if (here != 0 && under != 0 && here != under && rests[under] && !rests[here])
                {
                    rests[here] = true;
                    changed = true;
                }
            }
        }
    }
    bool moved = false;
    for (uint8_t label = 1; label <= count && !moved; label++)
    {
        moved = !rests[label];
    }
    if (!moved)
    {
        checkRows();
        return false;
    }
    for (int8_t y = ROWS - 2; y >= 0; y--)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            if (labels[y][x] != 0 && !rests[labels[y][x]])
            {
                well[y + 1][x] = well[y][x];
                well[y][x] = 0;
            }
        }
    }
    return true;
}

uint8_t Game::takeEvents()
{
    uint8_t taken = events;
    events = 0;
    return taken;
}
