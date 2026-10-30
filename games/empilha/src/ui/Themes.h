#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

// How a theme draws a block
enum class BlockStyle : uint8_t
{
    BEVEL,  // Raised, with a glint
    CRATE,  // Planks with a brace across
    SKETCH, // Hatched in pencil
    GLOW,   // Lit outline on a dark fill
    JELLY,  // Rounded, with a shine
    LCD     // A square in a square, one ink
};

struct ThemeDef
{
    const char *name; // Title value, 9 chars max
    BlockStyle style;
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// Every theme keeps CONFIRM at the physical button's red
static constexpr ThemeDef THEMES[] = {
    {"Slate", BlockStyle::BEVEL,
     {rgb(0x0a0e18), rgb(0x0e1420), rgb(0x182234), rgb(0x26344e), rgb(0x223048), rgb(0x4a6a9a),
      rgb(0xdce8ff), rgb(0x9ab0d8), rgb(0x5a6a8a), rgb(0x7ab8ff), rgb(0xffd070), rgb(0x7ae0a0), rgb(0xff7a7a),
      rgb(0xe06048), rgb(0x4a2a2a), rgb(0x7ab8ff), rgb(0x182234), rgb(0x4a6a9a),
      rgb(0x0a0e18), rgb(0x141c2c),
      rgb(0xff7a5c), rgb(0x3cc8b4), rgb(0xf0c040), rgb(0xa070e8), rgb(0x58a8f0), rgb(0xf070a8), rgb(0x9ad050),
      rgb(0xffffff), rgb(0x3a4660)}},
    {"Crate", BlockStyle::CRATE,
     {rgb(0x8a5a2c), rgb(0xd8b078), rgb(0xe4c490), rgb(0xf0d4a4), rgb(0xc09458), rgb(0x5a3414),
      rgb(0x3a1e08), rgb(0x6a3a14), rgb(0xa07840), rgb(0x8a2a0a), rgb(0x7a4a00), rgb(0x3a6a1a), rgb(0x9a1a0a),
      rgb(0xe06048), rgb(0xf0c0a0), rgb(0x6a3a14), rgb(0xe4c490), rgb(0x6a3a14),
      rgb(0x4a3018), rgb(0x54381e),
      rgb(0xe0b060), rgb(0xc08848), rgb(0xb0623c), rgb(0x9a9a58), rgb(0xd8c8a0), rgb(0xa8743c), rgb(0x8a8078),
      rgb(0xfff0c8), rgb(0x2e1c0c)}},
    {"Paper", BlockStyle::SKETCH,
     {rgb(0xece6d4), rgb(0xf6f1e2), rgb(0xefe9d6), rgb(0xe4dcc4), rgb(0xd4ccb4), rgb(0x3a4a7a),
      rgb(0x1e2a4a), rgb(0x3a4a7a), rgb(0x8a8a9a), rgb(0xc03030), rgb(0x8a6a10), rgb(0x2a6a3a), rgb(0xc03030),
      rgb(0xe06048), rgb(0xf0c8b8), rgb(0x3a4a7a), rgb(0xefe9d6), rgb(0x3a4a7a),
      rgb(0xfbf8ee), rgb(0xd8e0ec),
      rgb(0xc03030), rgb(0x2a7a3a), rgb(0x2a4ab0), rgb(0x8a3a1a), rgb(0x7a2a9a), rgb(0x2a7a8a), rgb(0x1e2a4a),
      rgb(0xf0c040), rgb(0xa0a0b0)}},
    {"Neon", BlockStyle::GLOW,
     {rgb(0x07060f), rgb(0x0b0a18), rgb(0x15132a), rgb(0x2a2050), rgb(0x2a2448), rgb(0x7a4cff),
      rgb(0xf0eaff), rgb(0xb8a8ff), rgb(0x6a5a9a), rgb(0xff3ad0), rgb(0xffe060), rgb(0x40ffb0), rgb(0xff4a6a),
      rgb(0xe06048), rgb(0x4a2030), rgb(0xff3ad0), rgb(0x15132a), rgb(0x7a4cff),
      rgb(0x05040c), rgb(0x100e20),
      rgb(0xff3ad0), rgb(0x40ffb0), rgb(0xffe060), rgb(0x40d8ff), rgb(0xff8a40), rgb(0xb070ff), rgb(0xff4a6a),
      rgb(0xffffff), rgb(0x2a2448)}},
    {"Candy", BlockStyle::JELLY,
     {rgb(0xf8c8dc), rgb(0xffd8e8), rgb(0xffe4ee), rgb(0xffc0d8), rgb(0xf0b0c8), rgb(0xb04a8a),
      rgb(0x5a1a4a), rgb(0x8a3a7a), rgb(0xc088b0), rgb(0xd02a8a), rgb(0xa06a00), rgb(0x2a8a5a), rgb(0xd02a4a),
      rgb(0xe06048), rgb(0xf8c0b0), rgb(0xd02a8a), rgb(0xffe4ee), rgb(0xb04a8a),
      rgb(0xfff4f8), rgb(0xfde4ee),
      rgb(0xf04a6a), rgb(0x2ab0b0), rgb(0xffb020), rgb(0x8a4ae0), rgb(0x3a9ae0), rgb(0xf070c0), rgb(0x6ac040),
      rgb(0xffffff), rgb(0xd8b8c8)}},
    {"Pocket", BlockStyle::LCD,
     {rgb(0x8a9a6a), rgb(0xa4b484), rgb(0x98a878), rgb(0x8a9a6a), rgb(0x8a9a6a), rgb(0x2a3a1e),
      rgb(0x1a2612), rgb(0x34462a), rgb(0x6a7a54), rgb(0x1a2612), rgb(0x2a3a1e), rgb(0x2a3a1e), rgb(0x1a2612),
      rgb(0xe06048), rgb(0xc0a080), rgb(0x2a3a1e), rgb(0x98a878), rgb(0x2a3a1e),
      rgb(0x98a878), rgb(0x8e9e6e),
      rgb(0x1a2612), rgb(0x2e3e22), rgb(0x24341a), rgb(0x3a4a2c), rgb(0x20301a), rgb(0x34462a), rgb(0x2a3a1e),
      rgb(0xd8e4b8), rgb(0x6a7a54)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

inline const ThemeDef &activeTheme()
{
    return THEMES[Theme::getActive()];
}

#endif // UI_THEMES_H
