#ifndef TILE_GRID_H
#define TILE_GRID_H

#include "UI.h"
#include "../../game/World.h"
#include "../../game/Colony.h"

// The habitat map. Every tile is composited whole (ground, critters, cursor) in one small
// sprite and pushed, so a critter walking across never leaves pixels behind; only tiles the
// world flagged as changed are repainted.
class TileGrid : public UI
{
public:
    TileGrid(int16_t x, int16_t y, World &world, const Colony &colony);

    void setCursor(uint8_t cx, uint8_t cy);
    // Advances the critters' bob and marks the grid dirty when a tile needs a repaint
    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

private:
    World &world;
    const Colony &colony;
    TFT_eSprite *sprite;
    uint8_t cursorX;
    uint8_t cursorY;
    bool bob;
    unsigned long nextBob;

    void paint(TFT_eSPI &tft, uint16_t i);
};

#endif // TILE_GRID_H
