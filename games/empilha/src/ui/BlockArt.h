#ifndef UI_BLOCK_ART_H
#define UI_BLOCK_ART_H

#include <TFT_eSPI.h>

// One cell, `s` pixels a side, in the active theme's style. Each paints its whole square, so a
// cell never needs erasing; `ground` is what shows round a block that does not fill it.
namespace BlockArt
{
    void block(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t type, uint16_t ground);
    void empty(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s);
    // Where the falling piece would land
    void ghost(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t type);
    void flash(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s);
    void dead(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s);
}

#endif // UI_BLOCK_ART_H
