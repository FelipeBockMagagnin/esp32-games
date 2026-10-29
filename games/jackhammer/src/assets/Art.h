#ifndef ART_H
#define ART_H

#include <Arduino.h>

// A sprite in palette indices, 0 transparent, frames stored one after the other. The palette
// and accents are byte-swapped RGB565, ready for the band buffers.
struct Art
{
    uint8_t w;
    uint8_t h;
    int8_t pivotX; // The point it turns around, which sits on the arena's centre
    int8_t pivotY;
    uint8_t frames;
    const uint16_t *palette;
    const uint8_t *pixels;
    uint16_t accents[2]; // Its two most used colours, for the particles of a kill
};

struct Backdrop
{
    uint8_t side; // Square, drawn at 2x
    const uint16_t *palette;
    const uint8_t *pixels;
};

// RGB565 byte-swapped, 0 transparent; tinted and scaled to each particle
struct ParticleTexture
{
    uint8_t w;
    uint8_t h;
    const uint16_t *pixels;
};

// White light over what is behind it, one alpha per pixel
struct Glow
{
    uint8_t w;
    uint16_t h;
    int16_t pivotX; // Where the character's pivot falls, possibly outside the image
    int16_t pivotY;
    const uint8_t *alpha;
};

#endif // ART_H
