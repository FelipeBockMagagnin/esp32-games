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

// Every theme keeps CONFIRM at the physical button's red
static constexpr ThemeDef THEMES[] = {
    {"Nogueira",
     {rgb(0x2a1c12), rgb(0x1a120c), rgb(0x261a10), rgb(0x4a3018), rgb(0x4a3420), rgb(0x8a6a40),
      rgb(0xf4e4c8), rgb(0xd0b088), rgb(0x8a7058), rgb(0xffb040), rgb(0xffc860), rgb(0x9ad060), rgb(0xff6a4a),
      rgb(0xe06048), rgb(0x5a2c20), rgb(0xffb040), rgb(0x2a1c12), rgb(0x8a6a40),
      rgb(0x6a3e1e), rgb(0x58321a), rgb(0x2e1a0c), rgb(0x1e1204), rgb(0xffb030), rgb(0x4a3010),
      rgb(0xc8c8c0), rgb(0x6a6a64), rgb(0x2a1a0e), rgb(0x60d060), rgb(0xf0d040), rgb(0xff5030)}},
    {"Cromo",
     {rgb(0x1c2228), rgb(0x101418), rgb(0x1a2026), rgb(0x2a3a4a), rgb(0x34404c), rgb(0x7a8a9a),
      rgb(0xeef2f6), rgb(0xb0c0d0), rgb(0x6a7a8a), rgb(0x60c0ff), rgb(0xffd870), rgb(0x80e0a0), rgb(0xff7060),
      rgb(0xe06048), rgb(0x4a2a2a), rgb(0x60c0ff), rgb(0x1c2228), rgb(0x7a8a9a),
      rgb(0x9aa2aa), rgb(0x8a929a), rgb(0x40484e), rgb(0x0a1a3a), rgb(0x80d8ff), rgb(0x1a3a60),
      rgb(0xe0e4e8), rgb(0x5a6066), rgb(0x1a2a3a), rgb(0x40d0ff), rgb(0x80ffe0), rgb(0xff60a0)}},
    {"Neon 80",
     {rgb(0x1a0c26), rgb(0x0a0612), rgb(0x160a22), rgb(0x3a1050), rgb(0x3a1a4a), rgb(0xa040c0),
      rgb(0xfff0ff), rgb(0xf0a0e0), rgb(0x7a4a8a), rgb(0x40f0ff), rgb(0xffe060), rgb(0x60ffb0), rgb(0xff5080),
      rgb(0xe06048), rgb(0x4a1a30), rgb(0xff40c0), rgb(0x1a0c26), rgb(0xa040c0),
      rgb(0x16101e), rgb(0x221830), rgb(0xff40c0), rgb(0x08001a), rgb(0xff50d0), rgb(0x3a1050),
      rgb(0xc0c0d8), rgb(0x50506a), rgb(0x1e1030), rgb(0x40f0ff), rgb(0xc060ff), rgb(0xff40a0)}},
    {"Marfim",
     {rgb(0xd4c4a0), rgb(0xe8dcc0), rgb(0xf0e6d0), rgb(0xd8c49a), rgb(0xc0ac88), rgb(0x7a6040),
      rgb(0x2a1e10), rgb(0x5a4428), rgb(0x9a8664), rgb(0xc05a20), rgb(0x9a6a10), rgb(0x2a7a3a), rgb(0xb02a1a),
      rgb(0xe06048), rgb(0xe8b0a0), rgb(0xc05a20), rgb(0xd4c4a0), rgb(0x7a6040),
      rgb(0xe0d0b0), rgb(0xd4c2a0), rgb(0x8a7454), rgb(0x1a2a18), rgb(0x90f070), rgb(0x2e4a28),
      rgb(0xd8d8d0), rgb(0x7a7a70), rgb(0xc8b898), rgb(0x3a9a3a), rgb(0xd09a20), rgb(0xc0301a)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

#endif // UI_THEMES_H
