#ifndef ARENA_RENDERER_H
#define ARENA_RENDERER_H

#include <Arduino.h>
#include "StyleArt.h"
#include "Effects.h"
#include "../jack/Round.h"

// Composites the square arena one horizontal band at a time into byte-swapped RGB565 ready to
// push: the background, the enemies, the jackhammer turned to its facing, then the particles.
// No frame buffer: the whole 240x240 would not fit in one allocation beside the rest.
class ArenaRenderer
{
public:
    static const int16_t BAND_H = 16;
    static const uint8_t BAND_COUNT = ARENA_PX / BAND_H;

    struct Scene
    {
        const StyleArt *style;
        uint16_t background; // RGB565, when the style has no backdrop
        const Round *round;
        const Effects *effects;
        uint8_t tick; // Advances the animated art
        bool showPlayer;
    };

    void renderBand(const Scene &scene, uint8_t band, uint16_t *out) const;

private:
    void background(const Scene &scene, int16_t top, uint16_t *out) const;
    // `turns` quarter turns clockwise around the art's pivot, which lands on (x, y)
    void blit(const Art &art, uint8_t frame, int16_t x, int16_t y, uint8_t turns, int16_t top, uint16_t *out) const;
    void particles(const Effects &effects, int16_t top, uint16_t *out) const;
};

#endif // ARENA_RENDERER_H
