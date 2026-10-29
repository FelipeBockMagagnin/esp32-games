#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

struct ThemeDef
{
    const char *name; // Settings value, 9 chars max
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// Every theme keeps CONFIRM at the physical button's red. The menus only: the 3D view's colours
// are art (render/View.cpp, art/).
static constexpr ThemeDef THEMES[] = {
    {"Crypt",
     {rgb(0x0c0a0e), rgb(0x141118), rgb(0x1f1a24), rgb(0x3a1c22), rgb(0x2e2834), rgb(0x5a4e62),
      rgb(0xe8e0d0), rgb(0xa89c90), rgb(0x5e5660), rgb(0xd8303a), rgb(0xe0b050), rgb(0x7cc070), rgb(0xe04848),
      rgb(0xe06048), rgb(0x5a2420),
      rgb(0x4878e0), rgb(0x141c34), rgb(0x2c3e70),
      rgb(0xc02a30), rgb(0x2a1012), rgb(0x6a1a1e),
      rgb(0x70b0f0), rgb(0xb070e8), rgb(0xf05050), rgb(0xf0c040)}},
    {"Ember",
     {rgb(0x100804), rgb(0x1a0e08), rgb(0x2a180e), rgb(0x4a2410), rgb(0x3a2414), rgb(0x7a4a24),
      rgb(0xf8e8d0), rgb(0xc8a080), rgb(0x6e5440), rgb(0xff8a30), rgb(0xffc850), rgb(0xa8d060), rgb(0xff5838),
      rgb(0xe06048), rgb(0x5a2c20),
      rgb(0x5890e8), rgb(0x141c30), rgb(0x2c4070),
      rgb(0xd83a28), rgb(0x30120a), rgb(0x701e12),
      rgb(0x78b8f0), rgb(0xc080f0), rgb(0xff6048), rgb(0xffd040)}},
    {"Frost",
     {rgb(0x080c12), rgb(0x0e141c), rgb(0x18222e), rgb(0x1e3a50), rgb(0x24303e), rgb(0x4a6278),
      rgb(0xe8f2ff), rgb(0x98aec4), rgb(0x52606e), rgb(0x60d0ff), rgb(0xf0d070), rgb(0x80e0a0), rgb(0xff6070),
      rgb(0xe06048), rgb(0x5a2c30),
      rgb(0x60a8ff), rgb(0x101c30), rgb(0x2a4a78),
      rgb(0xd83848), rgb(0x2a1018), rgb(0x6a1c2a),
      rgb(0x80c8ff), rgb(0xc090ff), rgb(0xff6878), rgb(0xffe070)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

#endif // UI_THEMES_H
