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

// The collectible shells of the toy. Every theme keeps CONFIRM at the physical button's red, and
// the dotui slots follow the LCD's colours so Settings looks like part of the same screen.
#define LCD_THEME(name, field, ghost, ink, mid, shell)                                                     \
    {                                                                                                      \
        name,                                                                                              \
        {                                                                                                  \
            rgb(ghost), rgb(field), rgb(ghost), rgb(mid), rgb(mid), rgb(ink), rgb(ink), rgb(ink), rgb(mid), \
                rgb(ink), rgb(ink), rgb(ink), rgb(0x8a2b16), rgb(0xe06048), rgb(0xd8a890), rgb(ink),       \
                rgb(ghost), rgb(mid), rgb(field), rgb(ghost), rgb(ink), rgb(shell)                         \
        }                                                                                                  \
    }

static constexpr ThemeDef THEMES[] = {
    LCD_THEME("Classic", 0xa9b78f, 0x9fae86, 0x1e2a1c, 0x75836a, 0x2f6f8f),
    LCD_THEME("White", 0xb9c0a6, 0xaeb69c, 0x22281f, 0x828a76, 0xece6d6),
    LCD_THEME("Pink", 0xc4bfa4, 0xb8b49a, 0x30222a, 0x8a7f78, 0xe8709c),
    LCD_THEME("Blue", 0xa8bcae, 0x9db1a4, 0x16283a, 0x70857f, 0x2a62c8),
    LCD_THEME("Yellow", 0xbcbf8c, 0xb1b483, 0x2a2612, 0x858760, 0xf0bc2a),
    LCD_THEME("Night", 0x101a2c, 0x18243a, 0x9fe8d0, 0x3a5470, 0x34466e),
};
static constexpr uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

#endif // UI_THEMES_H
