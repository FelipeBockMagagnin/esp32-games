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
    // Windows chrome
    SLOT_DESKTOP,
    SLOT_FACE, // Window and button face
    SLOT_TITLE_A, // Title bar gradient, left or top
    SLOT_TITLE_B,
    SLOT_TITLE_TEXT,
    SLOT_EDGE_HI, // Bevels: the lit edge, the shaded edge, the outer line
    SLOT_EDGE_LO,
    SLOT_EDGE_DARK,
    SLOT_FIELD, // Sunken fields: lists, text boxes, bar tracks
    SLOT_HILITE, // Selected row
    SLOT_HILITE_TEXT,
    SLOT_BAR, // Progress fill and its highlight
    SLOT_BAR_HI,
    SLOT_HP, // Health bars
    SLOT_HP_HI,
    SLOT_TASKBAR,
    SLOT_TASKBAR_HI,
    SLOT_START,
    SLOT_START_HI,
    SLOT_START_TEXT,
    SLOT_CLOSE,
    SLOT_TRAY,
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
    static constexpr Color DESKTOP = Color::slot(SLOT_DESKTOP);
    static constexpr Color FACE = Color::slot(SLOT_FACE);
    static constexpr Color TITLE_A = Color::slot(SLOT_TITLE_A);
    static constexpr Color TITLE_B = Color::slot(SLOT_TITLE_B);
    static constexpr Color TITLE_TEXT = Color::slot(SLOT_TITLE_TEXT);
    static constexpr Color EDGE_HI = Color::slot(SLOT_EDGE_HI);
    static constexpr Color EDGE_LO = Color::slot(SLOT_EDGE_LO);
    static constexpr Color EDGE_DARK = Color::slot(SLOT_EDGE_DARK);
    static constexpr Color FIELD = Color::slot(SLOT_FIELD);
    static constexpr Color HILITE = Color::slot(SLOT_HILITE);
    static constexpr Color HILITE_TEXT = Color::slot(SLOT_HILITE_TEXT);
    static constexpr Color BAR = Color::slot(SLOT_BAR);
    static constexpr Color BAR_HI = Color::slot(SLOT_BAR_HI);
    static constexpr Color HP = Color::slot(SLOT_HP);
    static constexpr Color HP_HI = Color::slot(SLOT_HP_HI);
    static constexpr Color TASKBAR = Color::slot(SLOT_TASKBAR);
    static constexpr Color TASKBAR_HI = Color::slot(SLOT_TASKBAR_HI);
    static constexpr Color START = Color::slot(SLOT_START);
    static constexpr Color START_HI = Color::slot(SLOT_START_HI);
    static constexpr Color START_TEXT = Color::slot(SLOT_START_TEXT);
    static constexpr Color CLOSE = Color::slot(SLOT_CLOSE);
    static constexpr Color TRAY = Color::slot(SLOT_TRAY);

    uint8_t getActive();
    // Out-of-range indices fall back to the first theme
    void setActive(uint8_t index);
}

#endif // UI_THEME_H
