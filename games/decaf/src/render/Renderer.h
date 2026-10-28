#ifndef RENDERER_H
#define RENDERER_H

#include <Arduino.h>
#include "../game/Stage.h"

// Composites the play area one horizontal band at a time, into a buffer of byte-swapped
// RGB565 ready to push: the office backdrop, the tiles, then the sprites. A band is one row of
// tiles, so the view never needs a frame buffer (320x224 would not fit in one allocation).
class Renderer
{
public:
    static const int16_t BAND_H = TILE;
    static const uint8_t BAND_COUNT = VIEW_H / BAND_H;

    // Once per frame, before the bands: gathers the sprites and the camera
    void beginFrame(const Stage &stage);
    void renderBand(const Stage &stage, uint8_t band, uint16_t *out) const;
    // For the PC tools, which film the floor from anywhere
    void setCamera(int16_t x) { camX = x; }

private:
    SpriteDraw draws[Stage::MAX_DRAWS];
    uint8_t drawCount = 0;
    int16_t camX = 0;
    uint16_t wallRows[VIEW_H]; // The backdrop's vertical gradient, swapped

    void backdrop(const Backdrop &b, int16_t top, uint16_t *out) const;
    void tiles(const Stage &stage, uint8_t band, uint16_t *out) const;
    void sprite(const SpriteDraw &d, int16_t top, uint16_t *out) const;
    void flag(const SpriteDraw &d, int16_t top, uint16_t *out) const;
};

#endif // RENDERER_H
