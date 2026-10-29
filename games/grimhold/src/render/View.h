#ifndef RENDER_VIEW_H
#define RENDER_VIEW_H

#include <stdint.h>
#include "Art.h"
#include "../crawl/Run.h"

// The first-person view, drawn a band of rows at a time into a buffer of byte-swapped RGB565, so
// the board sends one band by DMA while the next is drawn. Plain C++: the PC shots draw the same.
// The view is cast at half the horizontal resolution: a ray, and every texel, covers two pixels.
class View
{
public:
    static const int16_t W = 320;
    static const int16_t H = 192;
    static const int16_t COLS = W / 2;
    static const int16_t HORIZON = H / 2;

    // Casts the rays and lays out every sprite for the frame; drawBand only reads
    void prepare(const Run &run);
    // Rows [top, top + rows) of the view into `buffer`, W pixels a row
    void drawBand(uint16_t *buffer, int16_t top, int16_t rows) const;

private:
    static const uint8_t MAX_SPRITES = Run::MAX_ENEMIES + Run::MAX_PICKUPS + Run::MAX_PROJECTILES + Run::MAX_SPARKS;
    static const uint8_t MAX_OVERLAYS = 3;

    struct Column
    {
        float dist;
        const Bitmap *tex;
        uint32_t step; // Texel rows per screen row, 16.16
        int16_t top;   // Unclipped
        int16_t bottom;
        uint8_t texX;
        uint8_t row; // Shade row of the texture's palette
    };

    // A picture scaled into a screen rectangle; sprites are clipped by the walls' distances,
    // overlays (the hands) are not
    struct Quad
    {
        const Bitmap *bmp;
        float dist;
        int16_t left, top, width, height;
        int16_t c0, c1; // Columns covered, clipped
        uint32_t uStep, vStep;
        uint8_t row;
    };

    Column cols[COLS];
    Quad sprites[MAX_SPRITES];
    uint8_t spriteCount = 0;
    Quad overlays[MAX_OVERLAYS];
    uint8_t overlayCount = 0;
    uint16_t rowColor[H];
    uint8_t rowSet = 0xFF;
    bool hurt = false;
    int16_t bossFill = -1; // Width of the boss's health bar, -1 with no boss in the fight
    float lastX = 0, lastY = 0, bobPhase = 0;

    void castColumns(const Run &run);
    void paintRows(uint8_t set);
    void addSprite(const Run &run, const Bitmap &bmp, float x, float y, float size, float lift, uint8_t row);
    void addOverlay(const Bitmap &bmp, int16_t left, int16_t top, int16_t width, int16_t height, uint8_t row);
    void drawQuad(const Quad &q, uint16_t *buffer, int16_t top, int16_t rows, bool depth) const;
};

#endif // RENDER_VIEW_H
