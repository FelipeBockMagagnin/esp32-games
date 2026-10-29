#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

struct ThemeDef
{
    const char *name; // Settings value, 9 chars max
    const char *note; // Settings subtitle, 32 chars max
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// Every theme keeps CONFIRM at the physical button's red. The light ones put the whole screen
// on the LCD's colour; the dark ones are backlit tubes, the field a shade off the background.
static constexpr ThemeDef THEMES[] = {
    {"LCD", "Green pocket LCD",
     {rgb(0xa3bd7a), rgb(0xb8cf8f), rgb(0xaec784), rgb(0x94b06b), rgb(0x8fa96a), rgb(0x3e5229),
      rgb(0x1f2b14), rgb(0x3e5229), rgb(0x6b8250), rgb(0x1f2b14), rgb(0x4d4a12), rgb(0x2c5a1a), rgb(0x8a2b16),
      rgb(0xe06048), rgb(0xd8a890), rgb(0x3e5229), rgb(0xa3bd7a), rgb(0x6b8250),
      rgb(0xb8cf8f), rgb(0xabc383), rgb(0x24331a), rgb(0x24331a)}},
    {"Paper", "Grey reflective LCD",
     {rgb(0xb7b9ad), rgb(0xc9cbc0), rgb(0xc0c2b6), rgb(0xa8aa9e), rgb(0xa0a296), rgb(0x4a4c46),
      rgb(0x1a1c1a), rgb(0x474a44), rgb(0x7a7c72), rgb(0x1a1c1a), rgb(0x5a4a14), rgb(0x2a5a2a), rgb(0x8a2a1a),
      rgb(0xe06048), rgb(0xe0b0a0), rgb(0x4a4c46), rgb(0xb7b9ad), rgb(0x7a7c72),
      rgb(0xc9cbc0), rgb(0xbcbeb2), rgb(0x1e2020), rgb(0x1e2020)}},
    {"Ice", "Blue backlit LCD",
     {rgb(0x2a48b8), rgb(0x3657d0), rgb(0x3d60da), rgb(0x5378e8), rgb(0x5272e0), rgb(0xa8c0ff),
      rgb(0xeef4ff), rgb(0xb8ccff), rgb(0x8098e8), rgb(0xeef4ff), rgb(0xffe080), rgb(0xa0f0b0), rgb(0xffa090),
      rgb(0xe06048), rgb(0x60304c), rgb(0xb8ccff), rgb(0x2a48b8), rgb(0x8098e8),
      rgb(0x3657d0), rgb(0x3f60d8), rgb(0xeef4ff), rgb(0xeef4ff)}},
    {"Amber", "Amber terminal",
     {rgb(0x0e0801), rgb(0x140c02), rgb(0x24170a), rgb(0x3d2810), rgb(0x3a2810), rgb(0x7a5218),
      rgb(0xffc860), rgb(0xc08a30), rgb(0x6e5020), rgb(0xffb030), rgb(0xffd070), rgb(0xc8e060), rgb(0xff6040),
      rgb(0xe06048), rgb(0x5a2c20), rgb(0xc08a30), rgb(0x2a1b06), rgb(0x7a5218),
      rgb(0x1a1003), rgb(0x2c1d07), rgb(0xffb030), rgb(0xffe0a0)}},
    {"Phosphor", "Green phosphor CRT",
     {rgb(0x030803), rgb(0x050d06), rgb(0x0c1a0e), rgb(0x163a1c), rgb(0x143018), rgb(0x2c6a36),
      rgb(0xb0ffbc), rgb(0x60c070), rgb(0x306a3a), rgb(0x50ff78), rgb(0xe0ff80), rgb(0x50ff78), rgb(0xff5a4a),
      rgb(0xe06048), rgb(0x4a2c24), rgb(0x60c070), rgb(0x0c2212), rgb(0x2c6a36),
      rgb(0x07140a), rgb(0x0d2413), rgb(0x50ff78), rgb(0xd0ffc0)}},
    {"Pea", "Olive handheld, four shades",
     {rgb(0x7a8a32), rgb(0x8b9a3c), rgb(0x84943a), rgb(0x6e7e2c), rgb(0x6e7e2c), rgb(0x3e4c1a),
      rgb(0x263010), rgb(0x3e4c1a), rgb(0x5c6a26), rgb(0x263010), rgb(0x4a3e0c), rgb(0x1e4a14), rgb(0x7a2410),
      rgb(0xe06048), rgb(0xc89878), rgb(0x3e4c1a), rgb(0x7a8a32), rgb(0x5c6a26),
      rgb(0x8b9a3c), rgb(0x7f8e36), rgb(0x2f3a14), rgb(0x2f3a14)}},
    {"Pager", "Orange backlit LCD",
     {rgb(0xd88c34), rgb(0xf0a040), rgb(0xe89a3c), rgb(0xd08430), rgb(0xc88032), rgb(0x6a3a10),
      rgb(0x2e1806), rgb(0x5a3210), rgb(0x9a6020), rgb(0x2e1806), rgb(0x5a3a00), rgb(0x2a4a10), rgb(0x8a1a08),
      rgb(0xe06048), rgb(0xf0c0a0), rgb(0x6a3a10), rgb(0xd88c34), rgb(0x9a6020),
      rgb(0xf0a040), rgb(0xe09538), rgb(0x3a1e08), rgb(0x3a1e08)}},
    {"Calc", "Pale calculator LCD",
     {rgb(0xb0b8b4), rgb(0xc4ccc8), rgb(0xbcc4c0), rgb(0xa4aca8), rgb(0x9ca4a0), rgb(0x48505a),
      rgb(0x22282e), rgb(0x48505a), rgb(0x7c8480), rgb(0x22282e), rgb(0x5a4a14), rgb(0x2a5a3a), rgb(0x8a2a1a),
      rgb(0xe06048), rgb(0xe0b0a0), rgb(0x48505a), rgb(0xb0b8b4), rgb(0x7c8480),
      rgb(0xc4ccc8), rgb(0xb8c0bc), rgb(0x2a3038), rgb(0x2a3038)}},
    {"Crimson", "Red on black, stereo goggles",
     {rgb(0x060000), rgb(0x0a0000), rgb(0x1c0404), rgb(0x3a0808), rgb(0x340606), rgb(0x801414),
      rgb(0xff8070), rgb(0xd03030), rgb(0x701414), rgb(0xff2a2a), rgb(0xffb080), rgb(0xff9080), rgb(0xffe0c0),
      rgb(0xe06048), rgb(0x4a1010), rgb(0xd03030), rgb(0x260606), rgb(0x801414),
      rgb(0x120202), rgb(0x260606), rgb(0xff2a2a), rgb(0xff9080)}},
    {"Teal", "Cyan vector screen",
     {rgb(0x01070a), rgb(0x020a0c), rgb(0x08181c), rgb(0x10343a), rgb(0x0e2c32), rgb(0x1c6a70),
      rgb(0xb0fff8), rgb(0x50c0c0), rgb(0x24646a), rgb(0x40f0e8), rgb(0xf0f080), rgb(0x60ffa0), rgb(0xff6a5a),
      rgb(0xe06048), rgb(0x4a2c2a), rgb(0x50c0c0), rgb(0x0a2228), rgb(0x1c6a70),
      rgb(0x041216), rgb(0x0a2228), rgb(0x40f0e8), rgb(0xe0fff8)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

#endif // UI_THEMES_H
