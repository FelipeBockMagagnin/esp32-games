#ifndef FLUID_PALETTES_H
#define FLUID_PALETTES_H

#include <stdint.h>
#include "InkSim.h"

// The colours of the field, apart from the menus' theme. The ink is laid in each stop in turn,
// blending from one to the next as it flows; the water is a body colour with a lit rim along its
// surface and foam where it runs fast.
struct Palette
{
    const char *name; // Title value, 9 chars max
    uint32_t background;
    uint32_t ink[4];
    uint32_t water, rim, foam;
};

static const Palette PALETTES[] = {
    {"Neon", 0x05030c, {0xff2a8a, 0x22c8ff, 0xffd84a, 0x8a5cff}, 0x2a5cff, 0x9ff0ff, 0xffffff},
    {"Ocean", 0x020a14, {0x1a6cff, 0x10d0b0, 0xd8f4ff, 0x3a3ae0}, 0x0b5aa0, 0x70e0ff, 0xe8ffff},
    {"Lava", 0x0c0302, {0xff3b10, 0xff9a1a, 0xffe060, 0xc01850}, 0xc83814, 0xffc040, 0xfff0b0},
    {"Toxic", 0x020804, {0x40ff60, 0xd0ff20, 0x10c0a0, 0xa0ff90}, 0x30b030, 0xc8ff60, 0xf0fff0},
    {"Candy", 0x0a0610, {0xff80c0, 0x80d0ff, 0xfff080, 0xc0a0ff}, 0xc050d0, 0xffb0f0, 0xfff8ff},
    {"Mono", 0x000000, {0xffffff, 0x909090, 0xd0d0d0, 0x606060}, 0x808890, 0xe0e8f0, 0xffffff},
};
static const uint8_t PALETTE_COUNT = sizeof(PALETTES) / sizeof(PALETTES[0]);

inline Rgb toRgb(uint32_t hex)
{
    return {((hex >> 16) & 255) / 255.0f, ((hex >> 8) & 255) / 255.0f, (hex & 255) / 255.0f};
}

// The ink at `phase` along the palette's loop of stops, 0..1
inline Rgb inkAt(const Palette &p, float phase)
{
    phase -= (int)phase;
    float s = phase * 4;
    int a = (int)s;
    float f = s - a;
    f = f * f * (3 - 2 * f);
    Rgb c0 = toRgb(p.ink[a & 3]), c1 = toRgb(p.ink[(a + 1) & 3]);
    return {c0.r + (c1.r - c0.r) * f, c0.g + (c1.g - c0.g) * f, c0.b + (c1.b - c0.b) * f};
}

inline uint16_t rgb565(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

#endif // FLUID_PALETTES_H
