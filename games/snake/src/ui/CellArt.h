#ifndef UI_CELL_ART_H
#define UI_CELL_ART_H

#include <TFT_eSPI.h>
#include "Theme.h"
#include "../snake/Snake.h"

// One 10x10 cell of the dot-matrix field at (px, py). Every drawing repaints the whole cell,
// unlit dot included, so a cell never needs erasing first. The shapes are drawn facing right
// and turned for the other directions; a body joins its neighbours by reaching the cell's edge
// on their sides.
namespace CellArt
{
    static const int16_t SIZE = 10;

    inline uint8_t joinBit(Dir d) { return (uint8_t)(1 << (uint8_t)d); }

    void empty(TFT_eSPI &tft, int16_t px, int16_t py);
    // `joins` holds a joinBit() per neighbour; `scaled` punches the scale pattern of every other cell
    void body(TFT_eSPI &tft, int16_t px, int16_t py, uint8_t joins, bool fat, bool scaled);
    void tail(TFT_eSPI &tft, int16_t px, int16_t py, Dir toward);
    void head(TFT_eSPI &tft, int16_t px, int16_t py, Dir facing, bool mouthOpen, bool eyesShut);
    void food(TFT_eSPI &tft, int16_t px, int16_t py, bool big);
}

#endif // UI_CELL_ART_H
