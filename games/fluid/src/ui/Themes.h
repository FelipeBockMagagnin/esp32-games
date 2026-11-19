#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

struct ThemeDef
{
    const char *name; // Title value, 9 chars max
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// The menus' colours. Every theme keeps CONFIRM at the physical button's red.
static constexpr ThemeDef THEMES[] = {
    {"Abyss",
     {rgb(0x0a1a3a), rgb(0x050c1c), rgb(0x0e1c38), rgb(0x16305c), rgb(0x1c3260), rgb(0x3a68b0),
      rgb(0xe8f2ff), rgb(0x9ab8e0), rgb(0x52688c), rgb(0x60d0ff), rgb(0xffd860), rgb(0x60f0b0), rgb(0xff7060),
      rgb(0xe06048), rgb(0x4a2c38), rgb(0x60d0ff), rgb(0x0e1c38), rgb(0x3a68b0)}},
    {"Night",
     {rgb(0x181818), rgb(0x000000), rgb(0x141414), rgb(0x262626), rgb(0x2c2c2c), rgb(0x5a5a5a),
      rgb(0xf4f4f4), rgb(0xb0b0b0), rgb(0x6a6a6a), rgb(0xff4fa0), rgb(0xffd860), rgb(0x60f0b0), rgb(0xff7060),
      rgb(0xe06048), rgb(0x4a2c2c), rgb(0xff4fa0), rgb(0x141414), rgb(0x5a5a5a)}},
    {"Lagoon",
     {rgb(0x08443e), rgb(0x032824), rgb(0x0a3a34), rgb(0x105a50), rgb(0x0f5248), rgb(0x2a9a88),
      rgb(0xe8fff8), rgb(0x90d8c8), rgb(0x4a8a80), rgb(0x60ffe0), rgb(0xffe080), rgb(0x80ffa0), rgb(0xff8070),
      rgb(0xe06048), rgb(0x4a3030), rgb(0x60ffe0), rgb(0x0a3a34), rgb(0x2a9a88)}},
    {"Paper",
     {rgb(0xd8dde4), rgb(0xf2f4f6), rgb(0xe6eaee), rgb(0xd0d8e2), rgb(0xc4ccd6), rgb(0x6a7a8c),
      rgb(0x1a2430), rgb(0x46566a), rgb(0x8a96a4), rgb(0x1a6cff), rgb(0x8a6a00), rgb(0x1a7a40), rgb(0xb02a1a),
      rgb(0xe06048), rgb(0xf0c0b0), rgb(0x1a6cff), rgb(0xe6eaee), rgb(0x6a7a8c)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

#endif // UI_THEMES_H
