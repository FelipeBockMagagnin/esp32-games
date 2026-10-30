#include "WellView.h"
#include <string.h>
#include "BlockArt.h"
#include "Theme.h"

void WellView::place(int16_t x, int16_t y)
{
    ox = x;
    oy = y;
    invalidate();
}

void WellView::invalidate()
{
    memset(shown, SHOW_UNKNOWN, sizeof(shown));
}

void WellView::put(uint8_t x, uint8_t row, uint8_t what)
{
    if (shown[row][x] == what)
    {
        return;
    }
    shown[row][x] = what;
    int16_t px = ox + x * TILE, py = oy + row * TILE;
    if (what == SHOW_EMPTY)
    {
        BlockArt::empty(tft, px, py, TILE);
    }
    else if (what == SHOW_FLASH)
    {
        BlockArt::flash(tft, px, py, TILE);
    }
    else if (what == SHOW_DEAD)
    {
        BlockArt::dead(tft, px, py, TILE);
    }
    else if (what >= SHOW_GHOST)
    {
        BlockArt::ghost(tft, px, py, TILE, what - SHOW_GHOST);
    }
    else
    {
        BlockArt::block(tft, px, py, TILE, what - 1, Theme::WELL);
    }
}

void WellView::sync()
{
    uint8_t want[VISIBLE_ROWS][COLS];
    for (uint8_t row = 0; row < VISIBLE_ROWS; row++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            want[row][x] = game.cell(x, row + HIDDEN_ROWS);
        }
    }
    if (game.falling())
    {
        const Piece &piece = game.piece();
        Cell cells[PIECE_CELLS];
        Game::cellsOf(piece.type, piece.rot, cells);
        int8_t ghostY = game.ghostY();
        for (const Cell &c : cells)
        {
            int8_t row = ghostY + c.y - HIDDEN_ROWS;
            if (row >= 0)
            {
                want[row][piece.x + c.x] = SHOW_GHOST + piece.type;
            }
        }
        for (const Cell &c : cells)
        {
            int8_t row = piece.y + c.y - HIDDEN_ROWS;
            if (row >= 0)
            {
                want[row][piece.x + c.x] = piece.type + 1;
            }
        }
    }
    for (uint8_t row = 0; row < VISIBLE_ROWS; row++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            put(x, row, want[row][x]);
        }
    }
}

void WellView::wipe(uint32_t rows, uint8_t step)
{
    for (uint8_t row = 0; row < VISIBLE_ROWS; row++)
    {
        if (!(rows & (1UL << (row + HIDDEN_ROWS))))
        {
            continue;
        }
        for (uint8_t x = 0; x < COLS; x++)
        {
            uint8_t distance = x < COLS / 2 ? COLS / 2 - x : x - COLS / 2;
            if (distance == step)
            {
                put(x, row, SHOW_FLASH);
            }
            else if (distance < step)
            {
                put(x, row, SHOW_EMPTY);
            }
        }
    }
}

void WellView::fillRow(uint8_t row, bool won)
{
    for (uint8_t x = 0; x < COLS; x++)
    {
        put(x, row, won ? row % PIECE_COUNT + 1 : SHOW_DEAD);
    }
}
