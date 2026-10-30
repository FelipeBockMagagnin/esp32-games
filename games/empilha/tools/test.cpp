// Logic tests on the PC: the pieces, the bag, moving and turning, landing, clearing, scoring, the
// cascade, Sprint, topping out and the saved records. Build and run with tools/test.sh.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/stack/Game.h"
#include "../src/stack/Records.h"
#include <Preferences.h>

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0x12345678u;
static uint32_t xorshift()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static const uint8_t LONG_BAR = 0;
static const uint8_t SQUARE = 1;
static const uint8_t TEE = 2;
static const uint16_t LOCK_WAIT_MS = 500;

static Game game;

static int filled()
{
    int n = 0;
    for (uint8_t y = 0; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            n += game.cell(x, y) != 0;
        }
    }
    return n;
}

// Row y full but for the columns from `gapFrom`, `gapWidth` wide
static void fillRow(uint8_t y, uint8_t gapFrom = COLS, uint8_t gapWidth = 0)
{
    for (uint8_t x = 0; x < COLS; x++)
    {
        game.setCell(x, y, x >= gapFrom && x < gapFrom + gapWidth ? 0 : 1);
    }
}

static void place(uint8_t type, uint8_t rot, int8_t x, int8_t y)
{
    game.setPiece({type, rot, x, y});
    assert(game.fits(game.piece()));
}

static void piecesKeepTheirShapeWhenTurned()
{
    for (uint8_t type = 0; type < PIECE_COUNT; type++)
    {
        for (uint8_t rot = 0; rot < 4; rot++)
        {
            Cell cells[PIECE_CELLS];
            Game::cellsOf(type, rot, cells);
            uint8_t size = Game::boxSize(type);
            for (uint8_t i = 0; i < PIECE_CELLS; i++)
            {
                assert(cells[i].x >= 0 && cells[i].x < size && cells[i].y >= 0 && cells[i].y < size);
                for (uint8_t j = 0; j < i; j++)
                {
                    assert(cells[i].x != cells[j].x || cells[i].y != cells[j].y);
                }
            }
        }
        Cell before[PIECE_CELLS], after[PIECE_CELLS];
        Game::cellsOf(type, 0, before);
        Game::cellsOf(type, 4, after);
        assert(memcmp(before, after, sizeof(before)) == 0);
    }
}

static void everyPieceComesOncePerBag()
{
    game.reset(Mode::ENDLESS, xorshift);
    for (uint8_t bag = 0; bag < 20; bag++)
    {
        uint8_t seen = 0;
        for (uint8_t i = 0; i < PIECE_COUNT; i++)
        {
            seen |= 1 << game.piece().type;
            game.hardDrop();
            for (uint8_t y = 0; y < ROWS; y++)
            {
                for (uint8_t x = 0; x < COLS; x++)
                {
                    game.setCell(x, y, 0);
                }
            }
            game.clearRows();
        }
        assert(seen == 0x7F);
    }
}

static void piecesStartShowingAtTheTop()
{
    game.reset(Mode::ENDLESS, xorshift);
    for (uint8_t i = 0; i < 30; i++)
    {
        Cell cells[PIECE_CELLS];
        Game::cellsOf(game.piece().type, 0, cells);
        bool shows = false;
        for (const Cell &c : cells)
        {
            shows |= game.piece().y + c.y == HIDDEN_ROWS;
            assert(game.piece().y + c.y <= HIDDEN_ROWS);
        }
        assert(shows);
        game.hardDrop();
        for (uint8_t y = 0; y < ROWS; y++)
        {
            for (uint8_t x = 0; x < COLS; x++)
            {
                game.setCell(x, y, 0);
            }
        }
        game.clearRows();
    }
}

static void wallsStopThePiece()
{
    game.reset(Mode::ENDLESS, xorshift);
    while (game.move(-1))
    {
    }
    Cell cells[PIECE_CELLS];
    Game::cellsOf(game.piece().type, game.piece().rot, cells);
    int leftmost = COLS;
    for (const Cell &c : cells)
    {
        leftmost = game.piece().x + c.x < leftmost ? game.piece().x + c.x : leftmost;
    }
    assert(leftmost == 0);
    while (game.move(1))
    {
    }
    Game::cellsOf(game.piece().type, game.piece().rot, cells);
    int rightmost = 0;
    for (const Cell &c : cells)
    {
        rightmost = game.piece().x + c.x > rightmost ? game.piece().x + c.x : rightmost;
    }
    assert(rightmost == COLS - 1);
}

static void turningKicksOffTheWall()
{
    game.reset(Mode::ENDLESS, xorshift);
    place(LONG_BAR, 1, -2, 10);
    assert(game.rotate());
    assert(game.piece().x == 0);
    place(LONG_BAR, 1, COLS - 3, 10);
    assert(game.rotate());
    assert(game.fits(game.piece()));
}

static void theSquareNeverTurns()
{
    game.reset(Mode::ENDLESS, xorshift);
    place(SQUARE, 0, 4, 10);
    assert(!game.rotate());
}

static void aBlockedTurnDoesNothing()
{
    game.reset(Mode::ENDLESS, xorshift);
    for (uint8_t y = 0; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            game.setCell(x, y, 1);
        }
    }
    for (uint8_t x = 0; x < COLS; x++)
    {
        game.setCell(x, 11, 0);
    }
    place(LONG_BAR, 0, 3, 10);
    assert(!game.rotate());
    assert(game.piece().rot == 0 && game.piece().x == 3 && game.piece().y == 10);
}

static void gravityFollowsTheLevel()
{
    game.reset(Mode::ENDLESS, xorshift);
    int8_t y = game.piece().y;
    game.advance(799, false);
    assert(game.piece().y == y);
    game.advance(1, false);
    assert(game.piece().y == y + 1);

    game.reset(Mode::ENDLESS, xorshift);
    y = game.piece().y;
    game.advance(90, true);
    assert(game.piece().y == y + 3);

    game.reset(Mode::SPRINT, xorshift);
    y = game.piece().y;
    game.advance(350, false);
    assert(game.piece().y == y + 1);
}

static void theGhostIsWhereTheDropLands()
{
    game.reset(Mode::ENDLESS, xorshift);
    fillRow(ROWS - 1, 0, 1);
    place(TEE, 0, 4, 5);
    assert(game.ghostY() == ROWS - 3);
    game.hardDrop();
    assert(game.cell(5, ROWS - 2) == TEE + 1);
    assert(game.cell(4, ROWS - 2) == TEE + 1);
    assert(game.cell(5, ROWS - 3) == TEE + 1);
}

static void landingWaitsAndSlidingRestartsTheWait()
{
    game.reset(Mode::ENDLESS, xorshift);
    place(SQUARE, 0, 4, ROWS - 2);
    uint16_t landed = game.pieces();
    game.advance(400, false);
    assert(game.pieces() == landed);
    assert(game.move(1));
    game.advance(400, false);
    assert(game.pieces() == landed);
    game.advance(60, false);
    assert(game.pieces() == landed + 1);
}

static void slidingCannotStallForever()
{
    game.reset(Mode::ENDLESS, xorshift);
    place(SQUARE, 0, 4, ROWS - 2);
    uint16_t landed = game.pieces();
    for (uint8_t i = 0; i < 30 && game.pieces() == landed; i++)
    {
        game.move(i & 1 ? 1 : -1);
        game.advance(100, false);
    }
    assert(game.pieces() == landed + 1);
}

static void aClearDropsTheRowsAbove()
{
    game.reset(Mode::ENDLESS, xorshift);
    fillRow(ROWS - 1, 4, 2);
    fillRow(ROWS - 2, 4, 2);
    game.setCell(0, ROWS - 3, 3);
    place(SQUARE, 0, 4, ROWS - 2);
    game.hardDrop();
    assert(game.phase() == Game::Phase::CLEARING);
    assert(game.fullRows() == (3UL << (ROWS - 2)));
    assert(game.rows() == 2);
    game.clearRows();
    assert(game.falling());
    assert(game.cell(0, ROWS - 1) == 3);
    assert(filled() == 1);
}

static void scoresGrowWithRowsAndLevel()
{
    const uint32_t expected[] = {1, 3, 5, 9};
    for (uint8_t count = 1; count <= 4; count++)
    {
        game.reset(Mode::ENDLESS, xorshift);
        for (uint8_t i = 0; i < count; i++)
        {
            fillRow(ROWS - 1 - i, 5, 1);
        }
        place(LONG_BAR, 1, 3, ROWS - 4);
        game.hardDrop();
        assert(game.rows() == count);
        assert(game.score() == expected[count - 1]);
        game.clearRows();
    }
}

static void tenRowsRaiseTheLevel()
{
    game.reset(Mode::ENDLESS, xorshift);
    uint8_t rows = 0;
    while (rows < 12)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            fillRow(ROWS - 1 - i, 5, 1);
        }
        place(LONG_BAR, 1, 3, ROWS - 4);
        game.takeEvents();
        game.hardDrop();
        rows += 4;
        assert(((game.takeEvents() & Game::EVENT_LEVEL) != 0) == (rows == 12));
        assert(game.level() == 1 + rows / 10);
        game.clearRows();
    }
}

static void theCascadeDropsLooseClumpsAndChains()
{
    game.reset(Mode::CASCADE, xorshift);
    // The piece clears the bottom two rows. A loose pair above the third row's gap falls into it,
    // which completes that row
    fillRow(ROWS - 1, 4, 2);
    fillRow(ROWS - 2, 4, 2);
    fillRow(ROWS - 3, 9, 1);
    game.setCell(9, ROWS - 6, 5);
    game.setCell(9, ROWS - 5, 5);
    place(SQUARE, 0, 4, ROWS - 2);
    game.hardDrop();
    assert(game.chain() == 1);
    game.clearRows();
    assert(game.phase() == Game::Phase::SETTLING);
    uint8_t steps = 0;
    while (game.settleStep())
    {
        assert(++steps < ROWS * 2);
    }
    assert(game.phase() == Game::Phase::CLEARING);
    assert(game.chain() == 2);
    assert(game.rows() == 3);
    assert(game.score() == 3 + 1 * 2);
    game.clearRows();
    while (game.settleStep())
    {
    }
    assert(game.falling());
    assert(filled() == 1);
    assert(game.cell(9, ROWS - 1) == 5);
}

static void clumpsThatStillRestStayPut()
{
    game.reset(Mode::CASCADE, xorshift);
    fillRow(ROWS - 1, 4, 2);
    fillRow(ROWS - 2, 4, 2);
    // A floor that is not cleared, so what sits on it does not fall
    for (uint8_t x = 0; x < COLS; x++)
    {
        game.setCell(x, ROWS - 3, 0);
    }
    game.setCell(0, ROWS - 3, 2);
    game.setCell(0, ROWS - 4, 2);
    game.setCell(10, ROWS - 3, 3);
    place(SQUARE, 0, 4, ROWS - 2);
    game.hardDrop();
    game.clearRows();
    while (game.settleStep())
    {
    }
    assert(game.cell(0, ROWS - 1) == 2);
    assert(game.cell(0, ROWS - 2) == 2);
    assert(game.cell(10, ROWS - 1) == 3);
    assert(filled() == 3);
}

static void plainClearsNeverChain()
{
    game.reset(Mode::ENDLESS, xorshift);
    fillRow(ROWS - 1, 4, 2);
    fillRow(ROWS - 2, 4, 2);
    place(SQUARE, 0, 4, ROWS - 2);
    game.hardDrop();
    game.clearRows();
    assert(game.falling());
    assert(game.score() == 3);
}

static void sprintEndsAtThirtyRows()
{
    game.reset(Mode::SPRINT, xorshift);
    uint8_t rows = 0;
    while (rows < SPRINT_ROWS)
    {
        assert(game.falling());
        for (uint8_t i = 0; i < 4; i++)
        {
            fillRow(ROWS - 1 - i, 5, 1);
        }
        place(LONG_BAR, 1, 3, ROWS - 4);
        game.hardDrop();
        rows += 4;
        assert(game.level() == SPRINT_LEVEL);
        game.clearRows();
    }
    assert(game.phase() == Game::Phase::FINISHED);
    assert(game.rows() == 32);
}

static void theClockCountsThroughClearsAndStopsAtTheEnd()
{
    game.reset(Mode::SPRINT, xorshift);
    game.advance(300, false);
    assert(game.tenths() == 3);
    fillRow(ROWS - 1, 4, 2);
    fillRow(ROWS - 2, 4, 2);
    place(SQUARE, 0, 4, ROWS - 2);
    game.hardDrop();
    game.advance(100, false);
    assert(game.tenths() == 4);
    game.clearRows();

    for (uint8_t y = 0; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            game.setCell(x, y, y >= HIDDEN_ROWS && x != 0 ? 1 : 0);
        }
    }
    while (game.falling())
    {
        game.hardDrop();
    }
    assert(game.over());
    uint32_t stopped = game.tenths();
    game.advance(5000, false);
    assert(game.tenths() == stopped);
}

static void aBlockedSpawnTopsOut()
{
    game.reset(Mode::ENDLESS, xorshift);
    for (uint8_t y = HIDDEN_ROWS; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            game.setCell(x, y, x == 0 ? 0 : 1);
        }
    }
    uint8_t guard = 0;
    while (game.falling())
    {
        game.hardDrop();
        assert(++guard < 10);
    }
    assert(game.phase() == Game::Phase::TOPPED);
    assert(!game.move(1));
    assert(!game.rotate());
    game.hardDrop();
    assert(game.phase() == Game::Phase::TOPPED);
}

static void aThousandRandomGamesStayLegal()
{
    for (int run = 0; run < 1000; run++)
    {
        game.reset((Mode)(run % MODE_COUNT), xorshift);
        int cells = 0;
        for (int step = 0; step < 4000 && !game.over(); step++)
        {
            switch (game.phase())
            {
            case Game::Phase::CLEARING:
            {
                int removed = 0;
                for (uint8_t y = 0; y < ROWS; y++)
                {
                    removed += (game.fullRows() >> y & 1) * COLS;
                }
                game.clearRows();
                cells -= removed;
                break;
            }
            case Game::Phase::SETTLING:
                game.settleStep();
                break;
            default:
                assert(game.fits(game.piece()));
                switch (xorshift() % 5)
                {
                case 0:
                    game.move(-1);
                    break;
                case 1:
                    game.move(1);
                    break;
                case 2:
                    game.rotate();
                    break;
                case 3:
                    game.advance(16, xorshift() & 1);
                    break;
                default:
                {
                    uint16_t landed = game.pieces();
                    game.hardDrop();
                    assert(game.pieces() == landed + 1);
                    break;
                }
                }
                break;
            }
            if (game.takeEvents() & Game::EVENT_LANDED)
            {
                cells += PIECE_CELLS;
            }
            assert(game.score() <= SCORE_CAP);
            assert(game.level() >= 1 && game.level() <= MAX_LEVEL);
            assert(filled() == cells);
        }
    }
}

static void theBestKeepsTheBetterRun()
{
    Records records;
    defaultRecords(records);
    assert(!updateBest(records, Mode::ENDLESS, 0));
    assert(updateBest(records, Mode::ENDLESS, 120));
    assert(!updateBest(records, Mode::ENDLESS, 100));
    assert(updateBest(records, Mode::ENDLESS, 121));
    assert(updateBest(records, Mode::SPRINT, 900));
    assert(!updateBest(records, Mode::SPRINT, 901));
    assert(updateBest(records, Mode::SPRINT, 899));
    assert(records.best[(uint8_t)Mode::CASCADE] == 0);
}

static void theSaveSurvivesAndRefusesOtherVersions()
{
    SlotStore store("empilha");
    Records records;
    defaultRecords(records);
    records.mode = 2;
    records.best[0] = 4321;
    records.plays[2] = 7;
    assert(writeRecords(store, records));

    Records loaded;
    defaultRecords(loaded);
    assert(readRecords(store, loaded));
    assert(loaded.mode == 2 && loaded.best[0] == 4321 && loaded.plays[2] == 7);

    Records other = records;
    other.mode = 99;
    assert(writeRecords(store, other));
    assert(readRecords(store, loaded));
    assert(loaded.mode == 0);

    store.erase();
    defaultRecords(loaded);
    assert(!readRecords(store, loaded));
}

static void timesReadAsMinutes()
{
    char out[12];
    formatTime(0, out, sizeof(out));
    assert(strcmp(out, "-:--.-") == 0);
    formatTime(734, out, sizeof(out));
    assert(strcmp(out, "1:13.4") == 0);
}

int main()
{
    piecesKeepTheirShapeWhenTurned();
    everyPieceComesOncePerBag();
    piecesStartShowingAtTheTop();
    wallsStopThePiece();
    turningKicksOffTheWall();
    theSquareNeverTurns();
    aBlockedTurnDoesNothing();
    gravityFollowsTheLevel();
    theGhostIsWhereTheDropLands();
    landingWaitsAndSlidingRestartsTheWait();
    slidingCannotStallForever();
    aClearDropsTheRowsAbove();
    scoresGrowWithRowsAndLevel();
    tenRowsRaiseTheLevel();
    theCascadeDropsLooseClumpsAndChains();
    clumpsThatStillRestStayPut();
    plainClearsNeverChain();
    sprintEndsAtThirtyRows();
    theClockCountsThroughClearsAndStopsAtTheEnd();
    aBlockedSpawnTopsOut();
    aThousandRandomGamesStayLegal();
    theBestKeepsTheBetterRun();
    theSaveSurvivesAndRefusesOtherVersions();
    timesReadAsMinutes();
    puts("All tests passed");
    return 0;
}
