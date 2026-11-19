#ifndef FLUID_FIELD_RENDER_H
#define FLUID_FIELD_RENDER_H

#include <stdint.h>
#include "InkSim.h"
#include "Palettes.h"
#include "WaterSim.h"

// Turns a simulation into the panel's pixels, a band of rows at a time, in RGB565 high byte
// first (the order pushImage sends memory in). Both fields are coarse grids stretched over the
// screen by bilinear interpolation: the ink's dye straight, the water's density through a
// threshold that makes particles read as one smooth body.
class FieldRender
{
public:
    static const int FIELD_W = 240;
    static const int FIELD_H = 320;
    static const int WATER_CELL = 4;
    static const int WATER_W = FIELD_W / WATER_CELL;
    static const int WATER_H = FIELD_H / WATER_CELL;

    ~FieldRender() { endWater(); }

    void setPalette(const Palette &palette);

    void inkRows(const InkSim &sim, int top, int rows, uint16_t *out);

    // The water's grids, about 38 KB
    bool beginWater();
    void endWater();
    // Spreads the particles over the grids, once a frame before its rows
    void splat(const WaterSim &sim);
    void waterRows(int top, int rows, uint16_t *out);
    float waterLevel(int gx, int gy) const { return density[gx + gy * WATER_W]; } // For the tests

private:
    struct Axis
    {
        uint8_t first[FIELD_H]; // The lower of the two cells a pixel falls between
        float frac[FIELD_H];
    };

    float bgR = 0, bgG = 0, bgB = 0;
    uint16_t waterLut[64 * 8]; // By density level, then foam
    float *density = nullptr, *foam = nullptr;
    float line[3][InkSim::W > WATER_W ? InkSim::W : WATER_W];

    static void makeAxis(Axis &axis, int pixels, int cells, int scale);
    static Axis inkX, inkY, waterX, waterY;
    static bool axesReady;
    static void prepareAxes();
};

#endif // FLUID_FIELD_RENDER_H
