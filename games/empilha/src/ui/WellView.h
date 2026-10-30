#ifndef UI_WELL_VIEW_H
#define UI_WELL_VIEW_H

#include <TFT_eSPI.h>
#include "../stack/Game.h"

// Draws a Game on the panel. It remembers what each cell shows and sync() repaints only what
// differs, so a falling piece costs a dozen cells and never flickers. The clear and the top-out
// paint over that memory, and the next sync() puts the well back.
class WellView
{
public:
    static const int16_t TILE = 15;

    WellView(TFT_eSPI &tft, const Game &game) : tft(tft), game(game) {}

    void place(int16_t x, int16_t y);
    void invalidate();
    void sync();

    // One frame of the clear: the full rows vanish from the middle out, a bright edge leading.
    // Done once `step` reaches wipeSteps().
    void wipe(uint32_t rows, uint8_t step);
    static uint8_t wipeSteps() { return COLS / 2 + 2; }

    // Visible row `row`, counting from the top, at the end of a run: stripes in the pieces'
    // colours for a win, the dead stack otherwise
    void fillRow(uint8_t row, bool won);

private:
    enum Shown : uint8_t
    {
        SHOW_EMPTY = 0,
        // 1 to 7: a block of that type + 1
        SHOW_GHOST = 8, // 8 to 14: the ghost of that type
        SHOW_FLASH = 15,
        SHOW_DEAD = 16,
        SHOW_UNKNOWN = 0xFF
    };

    TFT_eSPI &tft;
    const Game &game;
    int16_t ox = 0, oy = 0;
    uint8_t shown[VISIBLE_ROWS][COLS];

    void put(uint8_t x, uint8_t row, uint8_t what);
};

#endif // UI_WELL_VIEW_H
