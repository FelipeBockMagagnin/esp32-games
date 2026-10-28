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

// Every UI colour on the panel, so screens share one look instead of each picking its own
// primaries. The values live in THEMES (Themes.h), one palette per theme, in this order.
// Terrain and critters keep their own palettes in WorldConfig.h: those are art.
enum ColorSlot : uint8_t
{
    // Surfaces, darkest to lightest in a dark theme
    SLOT_HEADER,       // title bar band
    SLOT_BG,           // screen background
    SLOT_CARD,         // cards, panels and list rows
    SLOT_ROW_SELECTED, // highlight behind the selected row
    // Borders
    SLOT_FRAME_DIM,  // unselected list rows, dividers
    SLOT_FRAME,      // cards, panels, header rule
    SLOT_GOLD_FRAME, // cards that show the gold balance
    // Text
    SLOT_TEXT,
    SLOT_TEXT_2,   // secondary lines and hints
    SLOT_TEXT_DIM, // locked, unavailable, cooling down
    // Meaning
    SLOT_SELECT, // selection frame
    SLOT_GOLD,   // gold amounts and prices
    SLOT_GOOD,   // affordable, owned, ready, better
    SLOT_BAD,    // worse, blocked, out of reach
    SLOT_INFO,   // new screens, guarding
    SLOT_EPIC,   // epic items, zone maps, spells
    SLOT_MYTHIC, // mythic items
    // Player and enemy stats, the same on every screen that shows them
    SLOT_ATTACK,
    SLOT_DEFENSE,
    SLOT_HP,
    SLOT_ENEMY_NAME, // warm: the enemy's side
    // The red cap of the physical Confirm button, the same in every theme
    SLOT_CONFIRM,
    SLOT_CONFIRM_DIM,
    // Bars: a fill with enough contrast for a TEXT label, a track of ~20% of it over the
    // background for the empty part, and a border at ~55%
    SLOT_GOLD_BAR, // progress toward a gold target
    SLOT_GOLD_TRACK,
    SLOT_GOLD_BORDER,
    SLOT_DONE_BAR, // a gold target reached
    SLOT_XP_BAR,   // the player's colour
    SLOT_XP_TRACK,
    SLOT_XP_BORDER,
    SLOT_HP_BAR,
    SLOT_HP_TRACK,
    SLOT_HP_BORDER,
    SLOT_ENEMY_HP_BAR,
    SLOT_ENEMY_HP_TRACK,
    SLOT_ENEMY_HP_BORDER,
    SLOT_ENEMY_ATK_BAR,
    SLOT_ENEMY_ATK_TRACK,
    SLOT_ENEMY_ATK_BORDER,
    SLOT_HIT_TRAIL, // health just lost, draining away
    // The enemy's stat card on the combat screen
    SLOT_ENEMY_FRAME,
    SLOT_ENEMY_BAND, // its title strip
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
    static constexpr Color GOLD_FRAME = Color::slot(SLOT_GOLD_FRAME);

    static constexpr Color TEXT = Color::slot(SLOT_TEXT);
    static constexpr Color TEXT_2 = Color::slot(SLOT_TEXT_2);
    static constexpr Color TEXT_DIM = Color::slot(SLOT_TEXT_DIM);

    static constexpr Color SELECT = Color::slot(SLOT_SELECT);
    static constexpr Color GOLD = Color::slot(SLOT_GOLD);
    static constexpr Color GOOD = Color::slot(SLOT_GOOD);
    static constexpr Color BAD = Color::slot(SLOT_BAD);
    static constexpr Color INFO = Color::slot(SLOT_INFO);
    static constexpr Color EPIC = Color::slot(SLOT_EPIC);
    static constexpr Color MYTHIC = Color::slot(SLOT_MYTHIC);

    static constexpr Color ATTACK = Color::slot(SLOT_ATTACK);
    static constexpr Color DEFENSE = Color::slot(SLOT_DEFENSE);
    static constexpr Color HP = Color::slot(SLOT_HP);
    static constexpr Color ENEMY_NAME = Color::slot(SLOT_ENEMY_NAME);
    static constexpr Color PLAYER_LINE = TEXT_2; // cool: the player's side

    static constexpr Color CONFIRM = Color::slot(SLOT_CONFIRM);
    static constexpr Color CONFIRM_DIM = Color::slot(SLOT_CONFIRM_DIM);

    static constexpr Color GOLD_BAR = Color::slot(SLOT_GOLD_BAR);
    static constexpr Color GOLD_TRACK = Color::slot(SLOT_GOLD_TRACK);
    static constexpr Color GOLD_BORDER = Color::slot(SLOT_GOLD_BORDER);
    static constexpr Color DONE_BAR = Color::slot(SLOT_DONE_BAR);
    static constexpr Color XP_BAR = Color::slot(SLOT_XP_BAR);
    static constexpr Color XP_TRACK = Color::slot(SLOT_XP_TRACK);
    static constexpr Color XP_BORDER = Color::slot(SLOT_XP_BORDER);
    static constexpr Color HP_BAR = Color::slot(SLOT_HP_BAR);
    static constexpr Color HP_TRACK = Color::slot(SLOT_HP_TRACK);
    static constexpr Color HP_BORDER = Color::slot(SLOT_HP_BORDER);
    static constexpr Color ENEMY_HP_BAR = Color::slot(SLOT_ENEMY_HP_BAR);
    static constexpr Color ENEMY_HP_TRACK = Color::slot(SLOT_ENEMY_HP_TRACK);
    static constexpr Color ENEMY_HP_BORDER = Color::slot(SLOT_ENEMY_HP_BORDER);
    static constexpr Color ENEMY_ATK_BAR = Color::slot(SLOT_ENEMY_ATK_BAR);
    static constexpr Color ENEMY_ATK_TRACK = Color::slot(SLOT_ENEMY_ATK_TRACK);
    static constexpr Color ENEMY_ATK_BORDER = Color::slot(SLOT_ENEMY_ATK_BORDER);
    static constexpr Color INERT_BAR = FRAME; // a full bar that does nothing
    static constexpr Color HIT_TRAIL = Color::slot(SLOT_HIT_TRAIL);

    static constexpr Color ENEMY_FRAME = Color::slot(SLOT_ENEMY_FRAME);
    static constexpr Color ENEMY_BAND = Color::slot(SLOT_ENEMY_BAND);

    uint8_t getActive();
    // Out-of-range indices fall back to the first theme
    void setActive(uint8_t index);
}

#endif // UI_THEME_H
