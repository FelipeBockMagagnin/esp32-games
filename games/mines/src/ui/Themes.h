#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

// How a theme draws its tiles: the covered one, the open one under it
enum class TileStyle : uint8_t
{
    BEVEL,  // Raised block, sunken grid
    FLAT,   // Rounded tile with a drop shadow
    GLOW,   // Outlined tile on a dark field
    SKETCH, // Hatched pencil square on graph paper
    GRASS,  // Lawn with tufts over soil
    WAVE    // Water with a ripple over sand
};

enum class MineGlyph : uint8_t
{
    BOMB,
    FLOWER,
    URCHIN,
    GEM
};

struct ThemeDef
{
    const char *name; // Title value, 9 chars max
    const char *note;
    TileStyle style;
    MineGlyph glyph;
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// Every theme keeps CONFIRM at the physical button's red
static constexpr ThemeDef THEMES[] = {
    {"Classic", "Grey tiles, raised edges", TileStyle::BEVEL, MineGlyph::BOMB,
     {rgb(0xc4c8cc), rgb(0xd4d7da), rgb(0xc8ccd0), rgb(0xb4bac2), rgb(0xa8aeb6), rgb(0x5a6470),
      rgb(0x1c2228), rgb(0x3c4650), rgb(0x7c868f), rgb(0x1f4fbf), rgb(0x8a6a00), rgb(0x1f7a2e), rgb(0xb02020),
      rgb(0xe06048), rgb(0xe8b0a0), rgb(0x1f4fbf), rgb(0xc8ccd0), rgb(0x5a6470),
      rgb(0x8c939a), rgb(0xc0c5ca), rgb(0xf2f4f6), rgb(0x7c838a), rgb(0xd4d7da), rgb(0xa6acb2),
      rgb(0x2050d8), rgb(0x1f8a30), rgb(0xd02828), rgb(0x1a2a8a), rgb(0x8a1e1e), rgb(0x1a8a8a), rgb(0x2a2a2a), rgb(0x6a6a6a),
      rgb(0x1a1c20), rgb(0xe8eaee), rgb(0xd82020), rgb(0xf0a020), rgb(0xff4a30)}},
    {"Night", "Slate tiles after dark", TileStyle::FLAT, MineGlyph::BOMB,
     {rgb(0x0a0e18), rgb(0x0e1420), rgb(0x182234), rgb(0x26344e), rgb(0x223048), rgb(0x4a6a9a),
      rgb(0xdce8ff), rgb(0x9ab0d8), rgb(0x5a6a8a), rgb(0x7ab8ff), rgb(0xffd070), rgb(0x7ae0a0), rgb(0xff7a7a),
      rgb(0xe06048), rgb(0x4a2a2a), rgb(0x7ab8ff), rgb(0x182234), rgb(0x4a6a9a),
      rgb(0x0a0e18), rgb(0x2a3850), rgb(0x3e5272), rgb(0x1a2436), rgb(0x121a28), rgb(0x1a2436),
      rgb(0x6aa8ff), rgb(0x6ae09a), rgb(0xff6a6a), rgb(0xc08aff), rgb(0xffaa5a), rgb(0x5ae0e0), rgb(0xe8eef8), rgb(0x8a9ab0),
      rgb(0xc8d4e8), rgb(0xffffff), rgb(0xffcf4a), rgb(0xffd070), rgb(0xff5a3a)}},
    {"Garden", "Flowers under the lawn", TileStyle::GRASS, MineGlyph::FLOWER,
     {rgb(0x4e7a34), rgb(0x5d8a3c), rgb(0x6a9846), rgb(0x7fae56), rgb(0x4a7030), rgb(0xe8f0c0),
      rgb(0xfbfff0), rgb(0xdfeab8), rgb(0x9fbf78), rgb(0xfff27a), rgb(0xffe060), rgb(0xc8ff90), rgb(0xff8a70),
      rgb(0xe06048), rgb(0x8a5a3a), rgb(0xfff27a), rgb(0x4e7a34), rgb(0xe8f0c0),
      rgb(0x3f6a2a), rgb(0x6cad3e), rgb(0x9fd65a), rgb(0x4a8a2c), rgb(0xb48a5a), rgb(0x9c7448),
      rgb(0x2850c8), rgb(0x1e6e1e), rgb(0xc02020), rgb(0x5a2a8a), rgb(0x8a3a10), rgb(0x106a6a), rgb(0x2a1a10), rgb(0x5a4a3a),
      rgb(0xff6a9a), rgb(0xffe040), rgb(0xff4040), rgb(0xfff27a), rgb(0xd03020)}},
    {"Ocean", "Urchins in the sand", TileStyle::WAVE, MineGlyph::URCHIN,
     {rgb(0x0c3a5a), rgb(0x0f4a70), rgb(0x155a84), rgb(0x1f7aa8), rgb(0x1a5a80), rgb(0x9ad8f0),
      rgb(0xf0fbff), rgb(0xb8e4f4), rgb(0x6aa8c8), rgb(0xffd66a), rgb(0xffd66a), rgb(0x8af0c0), rgb(0xff8a7a),
      rgb(0xe06048), rgb(0x6a3a40), rgb(0xffd66a), rgb(0x0c3a5a), rgb(0x9ad8f0),
      rgb(0x0a3352), rgb(0x1f7fb8), rgb(0x7fd0f0), rgb(0x15609a), rgb(0xe8d4a4), rgb(0xd4bc88),
      rgb(0x1a5ab0), rgb(0x1a7a4a), rgb(0xc83a2a), rgb(0x2a2a8a), rgb(0x8a2a1a), rgb(0x0a7a8a), rgb(0x3a2a1a), rgb(0x7a6a5a),
      rgb(0x4a1a5a), rgb(0xc070e0), rgb(0xff5a3a), rgb(0xffd66a), rgb(0xff6a4a)}},
    {"Desert", "Sandstone and cactus", TileStyle::BEVEL, MineGlyph::URCHIN,
     {rgb(0xc88a4a), rgb(0xd89c5a), rgb(0xe0a868), rgb(0xecbc80), rgb(0xc08048), rgb(0x6a3a14),
      rgb(0x3a1e08), rgb(0x6a3a14), rgb(0xa06a38), rgb(0x3a1e08), rgb(0x7a4a00), rgb(0x3a6a1a), rgb(0x9a1a0a),
      rgb(0xe06048), rgb(0xf0c0a0), rgb(0x6a3a14), rgb(0xe0a868), rgb(0x6a3a14),
      rgb(0xa8642c), rgb(0xd8a060), rgb(0xf4d09a), rgb(0xa86a34), rgb(0xf0d8a8), rgb(0xdcbc88),
      rgb(0x2a5aa8), rgb(0x3a7a2a), rgb(0xb82a1a), rgb(0x4a2a7a), rgb(0x7a2a0a), rgb(0x1a6a6a), rgb(0x3a2a1a), rgb(0x7a6040),
      rgb(0x3a7a3a), rgb(0xf0e080), rgb(0xc82a1a), rgb(0x2a6ad0), rgb(0xff6a2a)}},
    {"Candy", "Mint tiles, sugar gems", TileStyle::FLAT, MineGlyph::GEM,
     {rgb(0xf8c8dc), rgb(0xffd8e8), rgb(0xffe4ee), rgb(0xffc0d8), rgb(0xf0b0c8), rgb(0xb04a8a),
      rgb(0x5a1a4a), rgb(0x8a3a7a), rgb(0xc088b0), rgb(0xd02a8a), rgb(0xa06a00), rgb(0x2a8a5a), rgb(0xd02a4a),
      rgb(0xe06048), rgb(0xf8c0b0), rgb(0xd02a8a), rgb(0xffe4ee), rgb(0xb04a8a),
      rgb(0xffd8e8), rgb(0x8ad8e8), rgb(0xc8f4fa), rgb(0x5ab0c8), rgb(0xfff4f8), rgb(0xf8dce8),
      rgb(0x3a7ae0), rgb(0x2aa06a), rgb(0xf04a6a), rgb(0x8a4ae0), rgb(0xe0802a), rgb(0x2ab0b0), rgb(0x5a1a4a), rgb(0xa08aa0),
      rgb(0xd02a8a), rgb(0xffd0ec), rgb(0xff5a8a), rgb(0xffb020), rgb(0xff6a4a)}},
    {"Paper", "Pencil on graph paper", TileStyle::SKETCH, MineGlyph::BOMB,
     {rgb(0xece6d4), rgb(0xf6f1e2), rgb(0xefe9d6), rgb(0xe4dcc4), rgb(0xd4ccb4), rgb(0x3a4a7a),
      rgb(0x1e2a4a), rgb(0x3a4a7a), rgb(0x8a8a9a), rgb(0xc03030), rgb(0x8a6a10), rgb(0x2a6a3a), rgb(0xc03030),
      rgb(0xe06048), rgb(0xf0c8b8), rgb(0x3a4a7a), rgb(0xefe9d6), rgb(0x3a4a7a),
      rgb(0xf6f1e2), rgb(0xe6dfca), rgb(0xf6f1e2), rgb(0x6a7090), rgb(0xfbf8ee), rgb(0xc8d4e8),
      rgb(0x2a4ab0), rgb(0x2a7a3a), rgb(0xc03030), rgb(0x3a2a8a), rgb(0x8a3a1a), rgb(0x2a7a8a), rgb(0x1e2a4a), rgb(0x6a6a7a),
      rgb(0x1e2a4a), rgb(0xfbf8ee), rgb(0xc03030), rgb(0xc03030), rgb(0xf08a40)}},
    {"Neon", "Glowing outlines", TileStyle::GLOW, MineGlyph::GEM,
     {rgb(0x07060f), rgb(0x0b0a18), rgb(0x15132a), rgb(0x2a2050), rgb(0x2a2448), rgb(0x7a4cff),
      rgb(0xf0eaff), rgb(0xb8a8ff), rgb(0x6a5a9a), rgb(0xff3ad0), rgb(0xffe060), rgb(0x40ffb0), rgb(0xff4a6a),
      rgb(0xe06048), rgb(0x4a2030), rgb(0xff3ad0), rgb(0x15132a), rgb(0x7a4cff),
      rgb(0x0b0a18), rgb(0x1a1636), rgb(0x8a5cff), rgb(0x2e2660), rgb(0x0e0c1e), rgb(0x1e1a3a),
      rgb(0x40d8ff), rgb(0x40ff90), rgb(0xff4a8a), rgb(0xb070ff), rgb(0xffa040), rgb(0x40ffe0), rgb(0xffffff), rgb(0xa0a0c0),
      rgb(0xff3ad0), rgb(0xffd0f4), rgb(0x40ffb0), rgb(0xffe060), rgb(0xff6a40)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

inline const ThemeDef &activeTheme()
{
    return THEMES[Theme::getActive()];
}

#endif // UI_THEMES_H
