#ifndef UI_THEME_H
#define UI_THEME_H

#include <Arduino.h>

namespace Theme
{
    extern const uint16_t *activePalette;
}

// A UI colour: either a slot of the theme, looked up in the active palette each time it is
// drawn, or a fixed RGB565 value. Elements store Colors rather than RGB565, so they follow a
// theme change made after they were built.
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

// The values live in THEMES (Themes.h), one palette per theme, in this order
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
    SLOT_GOLD, // the theme's accent
    SLOT_GOOD,
    SLOT_BAD,
    SLOT_INFO,
    SLOT_EPIC,
    // The red cap of the physical Confirm button, the same in every theme
    SLOT_CONFIRM,
    SLOT_CONFIRM_DIM,
    // Bars: a fill with enough contrast for a TEXT label, a track of ~20% of it over the
    // background for the empty part, and a border at ~55%
    SLOT_GOLD_BAR,
    SLOT_GOLD_TRACK,
    SLOT_GOLD_BORDER,
    SLOT_DONE_BAR,
    SLOT_XP_BAR,
    SLOT_XP_TRACK,
    SLOT_XP_BORDER,
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
    static constexpr Color INFO = Color::slot(SLOT_INFO);
    static constexpr Color EPIC = Color::slot(SLOT_EPIC);

    static constexpr Color CONFIRM = Color::slot(SLOT_CONFIRM);
    static constexpr Color CONFIRM_DIM = Color::slot(SLOT_CONFIRM_DIM);

    static constexpr Color GOLD_BAR = Color::slot(SLOT_GOLD_BAR);
    static constexpr Color GOLD_TRACK = Color::slot(SLOT_GOLD_TRACK);
    static constexpr Color GOLD_BORDER = Color::slot(SLOT_GOLD_BORDER);
    static constexpr Color DONE_BAR = Color::slot(SLOT_DONE_BAR);
    static constexpr Color XP_BAR = Color::slot(SLOT_XP_BAR);
    static constexpr Color XP_TRACK = Color::slot(SLOT_XP_TRACK);
    static constexpr Color XP_BORDER = Color::slot(SLOT_XP_BORDER);

    uint8_t getActive();
    // Out-of-range indices fall back to the first theme
    void setActive(uint8_t index);
}

#endif // UI_THEME_H
