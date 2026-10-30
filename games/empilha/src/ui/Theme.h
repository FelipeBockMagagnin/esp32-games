#ifndef UI_THEME_H
#define UI_THEME_H

#include <Arduino.h>

namespace Theme
{
    extern const uint16_t *activePalette;
}

// A UI colour: either a slot of the theme, looked up in the active palette each time it is
// drawn, or a fixed RGB565 value (art). Elements store Colors rather than RGB565, so they
// follow a theme change made after they were built.
class Color
{
public:
    constexpr Color(uint16_t rgb = 0) : value(rgb), themed(false) {}
    static constexpr Color slot(uint8_t index) { return Color(index, true); }

    operator uint16_t() const { return themed ? Theme::activePalette[value] : value; }

    constexpr bool operator==(const Color &other) const { return value == other.value && themed == other.themed; }
    constexpr bool operator!=(const Color &other) const { return !(*this == other); }

private:
    constexpr Color(uint16_t v, bool t) : value(v), themed(t) {}

    uint16_t value;
    bool themed;
};

// The slots lib/dotui draws with, then the game's own. The values live in THEMES (Themes.h),
// one palette per theme, in this order.
enum ColorSlot : uint8_t
{
    SLOT_HEADER,
    SLOT_BG,
    SLOT_CARD,
    SLOT_ROW_SELECTED,
    SLOT_FRAME_DIM,
    SLOT_FRAME,
    SLOT_TEXT,
    SLOT_TEXT_2,
    SLOT_TEXT_DIM,
    SLOT_SELECT,
    SLOT_GOLD,
    SLOT_GOOD,
    SLOT_BAD,
    SLOT_CONFIRM, // The red cap of the physical Confirm button, the same in every theme
    SLOT_CONFIRM_DIM,
    SLOT_XP_BAR,
    SLOT_XP_TRACK,
    SLOT_XP_BORDER,
    // The well, its grid line, the seven pieces, a clearing row and the stack when it tops out
    SLOT_WELL,
    SLOT_GRID,
    SLOT_PIECE1,
    SLOT_PIECE2,
    SLOT_PIECE3,
    SLOT_PIECE4,
    SLOT_PIECE5,
    SLOT_PIECE6,
    SLOT_PIECE7,
    SLOT_FLASH,
    SLOT_DEAD,
    COLOR_SLOT_COUNT
};

namespace Theme
{
    static constexpr Color HEADER = Color::slot(SLOT_HEADER);
    static constexpr Color BG = Color::slot(SLOT_BG);
    static constexpr Color CARD = Color::slot(SLOT_CARD);
    static constexpr Color ROW_SELECTED = Color::slot(SLOT_ROW_SELECTED);
    static constexpr Color FRAME_DIM = Color::slot(SLOT_FRAME_DIM);
    static constexpr Color FRAME = Color::slot(SLOT_FRAME);
    static constexpr Color TEXT = Color::slot(SLOT_TEXT);
    static constexpr Color TEXT_2 = Color::slot(SLOT_TEXT_2);
    static constexpr Color TEXT_DIM = Color::slot(SLOT_TEXT_DIM);
    static constexpr Color SELECT = Color::slot(SLOT_SELECT);
    static constexpr Color GOLD = Color::slot(SLOT_GOLD);
    static constexpr Color GOOD = Color::slot(SLOT_GOOD);
    static constexpr Color BAD = Color::slot(SLOT_BAD);
    static constexpr Color CONFIRM = Color::slot(SLOT_CONFIRM);
    static constexpr Color CONFIRM_DIM = Color::slot(SLOT_CONFIRM_DIM);
    static constexpr Color XP_BAR = Color::slot(SLOT_XP_BAR);
    static constexpr Color XP_TRACK = Color::slot(SLOT_XP_TRACK);
    static constexpr Color XP_BORDER = Color::slot(SLOT_XP_BORDER);
    static constexpr Color WELL = Color::slot(SLOT_WELL);
    static constexpr Color GRID = Color::slot(SLOT_GRID);
    static constexpr Color FLASH = Color::slot(SLOT_FLASH);
    static constexpr Color DEAD = Color::slot(SLOT_DEAD);

    // 0 to 6
    inline Color piece(uint8_t type) { return Color::slot(SLOT_PIECE1 + type); }

    uint8_t getActive();
    // Out-of-range indices fall back to the first theme
    void setActive(uint8_t index);
}

#endif // UI_THEME_H
