#ifndef UI_TILE_MAP_H
#define UI_TILE_MAP_H

#include "UI.h"
#include "../rogue/Run.h"

// The dungeon floor, one 16x16 sprite per cell with whatever stands on it composited over the
// tile. It remembers what each cell last showed and repaints only the cells that changed, so
// a step costs two cells, not the map; a full repaint (a new floor, the screen re-entered)
// happens when the screen was cleared under it.
class TileMap : public UI
{
public:
    static const int16_t TILE = 16;

    TileMap(int16_t x, int16_t y, const Run &run) : UI(x, y, MAP_W * TILE, MAP_H * TILE), run(run) {}

    void draw(TFT_eSPI &tft) override;

private:
    const Run &run;
    uint8_t shown[MAP_H][MAP_W] = {}; // Tile * GLYPH_COUNT + glyph

    void paintCell(TFT_eSPI &tft, uint8_t cx, uint8_t cy, Tile tile, Glyph glyph);
};

#endif // UI_TILE_MAP_H
