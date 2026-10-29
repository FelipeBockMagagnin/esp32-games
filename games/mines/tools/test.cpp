// Logic tests on the PC: laying the mines, flooding, chords, flags, winning and the saved
// records. Build and run with tools/test.sh.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/mines/Field.h"
#include "../src/mines/Records.h"
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

static Field field;

static int mineTotal()
{
    int n = 0;
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            n += field.isMine({x, y});
        }
    }
    return n;
}

static void firstOpenIsSafeAndFloods()
{
    for (uint8_t l = 0; l < LEVEL_COUNT; l++)
    {
        for (int round = 0; round < 200; round++)
        {
            const Level &lv = LEVELS[l];
            field.reset(lv.cols, lv.rows, lv.mines, xorshift);
            Cell c = {(uint8_t)(xorshift() % lv.cols), (uint8_t)(xorshift() % lv.rows)};
            Field::Result r = field.open(c);
            assert(r == Field::OPENED || r == Field::WON);
            assert(field.armed());
            assert(mineTotal() == lv.mines);
            assert(field.count(c) == 0);
            Cell around[8];
            uint8_t n = field.neighbours(c, around);
            for (uint8_t i = 0; i < n; i++)
            {
                assert(!field.isMine(around[i]));
                assert(field.isOpen(around[i]));
            }
            assert(field.openedCount() >= n + 1u);
        }
    }
    puts("first open is safe and floods");
}

static void countsMatchNeighbours()
{
    field.reset(14, 18, 50, xorshift);
    field.open({7, 9});
    for (uint8_t y = 0; y < 18; y++)
    {
        for (uint8_t x = 0; x < 14; x++)
        {
            Cell around[8];
            uint8_t n = field.neighbours({x, y}, around), m = 0;
            for (uint8_t i = 0; i < n; i++)
            {
                m += field.isMine(around[i]);
            }
            assert(field.count({x, y}) == m);
        }
    }
    puts("counts match the mines around");
}

// Waves never skip a step, and each cell is reached from an empty one of the wave before
static void wavesAreFloodDistances()
{
    for (int round = 0; round < 100; round++)
    {
        field.reset(12, 16, 30, xorshift);
        field.open({(uint8_t)(xorshift() % 12), (uint8_t)(xorshift() % 16)});
        assert(field.opened(0).wave == 0);
        for (uint16_t i = 1; i < field.openedCount(); i++)
        {
            const Field::Opened &o = field.opened(i);
            assert(o.wave >= field.opened(i - 1).wave);
            assert(o.wave <= field.opened(i - 1).wave + 1);
            bool fed = false;
            for (uint16_t j = 0; j < i; j++)
            {
                const Field::Opened &p = field.opened(j);
                int dx = abs(p.cell.x - o.cell.x), dy = abs(p.cell.y - o.cell.y);
                if (p.wave + 1 == o.wave && dx <= 1 && dy <= 1 && field.count(p.cell) == 0)
                {
                    fed = true;
                }
            }
            assert(fed);
            assert(!field.isMine(o.cell));
        }
    }
    puts("waves are flood distances");
}

static bool findNumber(Cell &out)
{
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            Cell c = {x, y};
            if (!field.isOpen(c) || field.count(c) == 0)
            {
                continue;
            }
            Cell around[8];
            uint8_t n = field.neighbours(c, around), covered = 0;
            for (uint8_t i = 0; i < n; i++)
            {
                covered += !field.isOpen(around[i]) && !field.isMine(around[i]);
            }
            if (covered > 0)
            {
                out = c;
                return true;
            }
        }
    }
    return false;
}

static void chords()
{
    int right = 0, wrong = 0;
    for (int round = 0; round < 200 && (right < 20 || wrong < 20); round++)
    {
        field.reset(14, 18, 50, xorshift);
        field.open({7, 9});
        Cell c;
        if (!findNumber(c))
        {
            continue;
        }
        Cell around[8];
        uint8_t n = field.neighbours(c, around);
        assert(field.chord(c) == Field::NOTHING);

        // A right guess flags the mines around; a wrong one swaps one mine for a safe cell
        bool flagWrong = (round & 1) != 0;
        Cell safe = c;
        for (uint8_t i = 0; i < n; i++)
        {
            if (!field.isOpen(around[i]) && !field.isMine(around[i]))
            {
                safe = around[i];
            }
        }
        bool swapped = false;
        for (uint8_t i = 0; i < n; i++)
        {
            if (!field.isMine(around[i]))
            {
                continue;
            }
            if (flagWrong && !swapped)
            {
                field.toggleFlag(safe);
                swapped = true;
                continue;
            }
            field.toggleFlag(around[i]);
        }
        bool reallyWrong = swapped;
        Field::Result r = field.chord(c);
        if (reallyWrong)
        {
            assert(r == Field::HIT);
            assert(field.isMine(field.hitCell()));
            assert(field.over());
            wrong++;
        }
        else
        {
            assert(r == Field::OPENED || r == Field::WON);
            for (uint8_t i = 0; i < n; i++)
            {
                assert(field.isOpen(around[i]) || field.isFlagged(around[i]));
            }
            right++;
        }
    }
    assert(right >= 10 && wrong >= 10);
    puts("chords open with the right flags and blow up on a wrong one");
}

static void flags()
{
    field.reset(9, 12, 10, xorshift);
    assert(field.minesLeft() == 10);
    assert(field.toggleFlag({0, 0}));
    assert(field.isFlagged({0, 0}));
    assert(field.minesLeft() == 9);
    assert(field.open({0, 0}) == Field::NOTHING);
    assert(!field.isOpen({0, 0}));
    assert(field.toggleFlag({0, 0}));
    assert(field.minesLeft() == 10);
    field.open({4, 6});
    assert(!field.toggleFlag({4, 6}));
    puts("flags toggle, block opening and count down");
}

static void winning()
{
    for (uint8_t l = 0; l < LEVEL_COUNT; l++)
    {
        const Level &lv = LEVELS[l];
        field.reset(lv.cols, lv.rows, lv.mines, xorshift);
        field.open({0, 0});
        Field::Result r = Field::OPENED;
        for (uint8_t y = 0; y < lv.rows && r != Field::WON; y++)
        {
            for (uint8_t x = 0; x < lv.cols && r != Field::WON; x++)
            {
                Cell c = {x, y};
                if (!field.isMine(c) && !field.isOpen(c))
                {
                    assert(field.safeLeft() > 0);
                    r = field.open(c);
                }
            }
        }
        assert(r == Field::WON);
        assert(field.isWon() && field.over());
        assert(field.safeLeft() == 0);
        assert(field.minesLeft() == 0);
        assert(field.open({0, 0}) == Field::NOTHING);
    }
    puts("opening every safe cell wins and flags the mines");
}

static void hitting()
{
    field.reset(9, 12, 10, xorshift);
    field.open({4, 6});
    for (uint8_t y = 0; y < 12; y++)
    {
        for (uint8_t x = 0; x < 9; x++)
        {
            if (field.isMine({x, y}))
            {
                assert(field.open({x, y}) == Field::HIT);
                assert(field.hitCell() == (Cell{x, y}));
                assert(field.openedCount() == 1);
                assert(!field.toggleFlag({0, 0}));
                puts("opening a mine ends the game");
                return;
            }
        }
    }
    assert(false);
}

static void records()
{
    SlotStore store("minestest");
    store.erase();
    Records r;
    defaultRecords(r);
    assert(!readRecords(store, r));
    r.level = 2;
    r.best[1] = 1234;
    r.wins[2] = 7;
    r.plays[2] = 9;
    assert(writeRecords(store, r));
    Records back;
    defaultRecords(back);
    assert(readRecords(store, back));
    assert(back.level == 2 && back.best[1] == 1234 && back.wins[2] == 7 && back.plays[2] == 9);

    Records old = r;
    old.version = RECORDS_VERSION + 1;
    store.save(&old, sizeof(old));
    Records kept;
    defaultRecords(kept);
    assert(!readRecords(store, kept));
    assert(kept.level == 0);

    char buf[12];
    formatTime(0, buf, sizeof(buf));
    assert(strcmp(buf, "-:--.-") == 0);
    formatTime(1234, buf, sizeof(buf));
    assert(strcmp(buf, "2:03.4") == 0);
    formatTime(65000, buf, sizeof(buf));
    assert(strcmp(buf, "99:59.9") == 0);
    puts("records round-trip and drop another version");
}

int main()
{
    firstOpenIsSafeAndFloods();
    countsMatchNeighbours();
    wavesAreFloodDistances();
    chords();
    flags();
    winning();
    hitting();
    records();
    puts("all passed");
    return 0;
}
