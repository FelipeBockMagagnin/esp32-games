#ifndef UI_THEMES_H
#define UI_THEMES_H

#include "Theme.h"

// How a theme draws its windows (ui/Win.cpp), beyond the colours
enum class WinStyle : uint8_t
{
    CLASSIC, // Windows 98: square 2px bevels, a left-to-right title gradient, block bars
    LUNA,    // Windows XP: rounded blue title, green Start button, green block bars
    AERO     // Windows 7: pale glass title, round Start orb, smooth bars with a shine
};

struct ThemeDef
{
    const char *name; // Settings value, 9 chars max
    WinStyle style;
    uint16_t colors[COLOR_SLOT_COUNT]; // RGB565, in ColorSlot order
};

static constexpr uint16_t rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 0x1F) << 11 | ((hex >> 10) & 0x3F) << 5 | ((hex >> 3) & 0x1F));
}

// Columns: HEADER BG CARD ROW_SELECTED FRAME_DIM FRAME TEXT TEXT_2 TEXT_DIM SELECT GOLD GOOD BAD
// CONFIRM CONFIRM_DIM XP_BAR XP_TRACK XP_BORDER, then DESKTOP FACE TITLE_A TITLE_B TITLE_TEXT
// EDGE_HI EDGE_LO EDGE_DARK FIELD HILITE HILITE_TEXT BAR BAR_HI HP HP_HI TASKBAR TASKBAR_HI START
// START_HI START_TEXT CLOSE TRAY
static constexpr ThemeDef THEMES[] = {
    {"Win 98", WinStyle::CLASSIC,
     {rgb(0x000080), rgb(0xc0c0c0), rgb(0xffffff), rgb(0x000080), rgb(0x808080), rgb(0x000000),
      rgb(0x000000), rgb(0x404040), rgb(0x808080), rgb(0x000080), rgb(0x806000), rgb(0x007000), rgb(0xb00000),
      rgb(0xe06048), rgb(0xe8b0a0), rgb(0x000080), rgb(0xffffff), rgb(0x808080),
      rgb(0x008080), rgb(0xc0c0c0), rgb(0x000080), rgb(0x1084d0), rgb(0xffffff),
      rgb(0xffffff), rgb(0x808080), rgb(0x000000), rgb(0xffffff), rgb(0x000080), rgb(0xffffff),
      rgb(0x000080), rgb(0x000080), rgb(0xa00000), rgb(0xa00000), rgb(0xc0c0c0), rgb(0xffffff), rgb(0xc0c0c0),
      rgb(0xffffff), rgb(0x000000), rgb(0xc0c0c0), rgb(0xc0c0c0)}},
    {"Win XP", WinStyle::LUNA,
     {rgb(0x0054e3), rgb(0xece9d8), rgb(0xffffff), rgb(0x316ac5), rgb(0xaca899), rgb(0x003c74),
      rgb(0x000000), rgb(0x404040), rgb(0x8a877a), rgb(0x316ac5), rgb(0x9a6a00), rgb(0x1e7a1e), rgb(0xc02020),
      rgb(0xe06048), rgb(0xe8b0a0), rgb(0x31b331), rgb(0xffffff), rgb(0x7f9db9),
      rgb(0x3a6ea5), rgb(0xece9d8), rgb(0x3d95ff), rgb(0x0046d5), rgb(0xffffff),
      rgb(0xffffff), rgb(0xaca899), rgb(0x003c74), rgb(0xffffff), rgb(0x316ac5), rgb(0xffffff),
      rgb(0x2eb52e), rgb(0x9de89d), rgb(0xc62b2b), rgb(0xf08a8a), rgb(0x245edb), rgb(0x5a9cf8), rgb(0x3c9a3c),
      rgb(0x6fd06f), rgb(0xffffff), rgb(0xe0442a), rgb(0x0f8ff2)}},
    {"Win 7", WinStyle::AERO,
     {rgb(0x8fb2da), rgb(0xf0f0f0), rgb(0xffffff), rgb(0xcce8ff), rgb(0xa0a0a0), rgb(0x3c5a7a),
      rgb(0x000000), rgb(0x3c3c3c), rgb(0x8c8c8c), rgb(0x3399ff), rgb(0x8a5a00), rgb(0x0a7a2a), rgb(0xc02828),
      rgb(0xe06048), rgb(0xe8b0a0), rgb(0x06b025), rgb(0xe6e6e6), rgb(0xbcbcbc),
      rgb(0x1e5a96), rgb(0xf0f0f0), rgb(0xc4daf2), rgb(0x8cb0dc), rgb(0x0a1a2a),
      rgb(0xffffff), rgb(0xb4b4b4), rgb(0x3c5a7a), rgb(0xffffff), rgb(0xcce8ff), rgb(0x000000),
      rgb(0x06b025), rgb(0x8ae69a), rgb(0xd03030), rgb(0xf09090), rgb(0x1c3348), rgb(0x4a6a8a), rgb(0x2a7ad0),
      rgb(0x9ed2fa), rgb(0xffffff), rgb(0xc7504a), rgb(0x2a4560)}},
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

inline const ThemeDef &activeTheme()
{
    return THEMES[Theme::getActive()];
}

#endif // UI_THEMES_H
