#ifndef UI_TILE_ART_H
#define UI_TILE_ART_H

#include <TFT_eSPI.h>

// One tile of the minefield, `s` pixels a side, in the active theme's style. Every tile paints
// its whole square, so a tile never needs erasing; the overlays (cursor, ring, sparkle) are
// drawn on top of one and cleared by drawing the tile again. `parity` varies the texture from
// tile to tile so the lawn and the water do not repeat in lockstep.
namespace TileArt
{
    static const uint8_t FLAG_FRAMES = 3; // The last one is the flag at rest
    static const uint8_t POP_FRAMES = 2;  // A covered tile sinking open, before the open tile
    static const uint8_t RING_DOTS = 12;

    void covered(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity);
    void open(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t count);
    void popping(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t frame);
    void flag(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t frame);
    void wrongFlag(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity);
    // `blasted` puts it on the blast colour; `lit` is the flash of the blast
    void mine(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, bool blasted, bool lit);

    void cursor(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, bool tight);
    void ring(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t dots);
    void sparkle(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s);

    // The glyphs alone, centred on (cx, cy), for the HUD and the title
    void mineGlyph(TFT_eSPI &tft, int16_t cx, int16_t cy, int16_t s);
    void flagGlyph(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t frame);
}

#endif // UI_TILE_ART_H
