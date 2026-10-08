#ifndef UI_THEME_H
#define UI_THEME_H

#include <Arduino.h>

// Every UI colour on the panel, so screens share one look instead of each picking its
// own primaries. The "cave" palette: warm charcoal surfaces, cream text, gold and copper
// accents, with the cool colours (defense, info) kept muted so they don't fight the gold.
// RGB565, with the 24-bit source alongside. Ore tiers keep their own palettes in
// GameConfig.h: those are art.
namespace Theme
{
    // Surfaces, darkest to lightest
    static constexpr uint16_t HEADER = 0x0841;       // #0D0B0A title bar band
    static constexpr uint16_t BG = 0x1081;           // #14110F screen background
    static constexpr uint16_t CARD = 0x20E3;         // #221C18 cards, panels and list rows
    static constexpr uint16_t ROW_SELECTED = 0x3964; // #3A2E24 highlight behind the selected row

    // Borders
    static constexpr uint16_t FRAME_DIM = 0x2923;  // #2C241E unselected list rows, dividers
    static constexpr uint16_t FRAME = 0x49E6;      // #4A3D32 cards, panels, header rule
    static constexpr uint16_t GOLD_FRAME = 0x9B45; // #9C6A2E cards that show the gold balance

    // Text
    static constexpr uint16_t TEXT = 0xF75B;     // #F2E8D8
    static constexpr uint16_t TEXT_2 = 0xBD52;   // #B8A890 secondary lines and hints
    static constexpr uint16_t TEXT_DIM = 0x6B0A; // #6E6252 locked, unavailable, cooling down

    // Meaning
    static constexpr uint16_t SELECT = 0xF5A8; // #F2B544 selection frame
    static constexpr uint16_t GOLD = 0xF5A8;   // #F2B544 gold amounts and prices
    static constexpr uint16_t GOOD = 0x8E4D;   // #8FCB6B affordable, owned, ready, better
    static constexpr uint16_t BAD = 0xE309;    // #E0604E worse, blocked, out of reach
    static constexpr uint16_t INFO = 0x5E39;   // #5EC4C9 new screens, guarding
    static constexpr uint16_t EPIC = 0xC45B;   // #C58AD8 epic items, zone maps, smite

    // Player and enemy stats, the same on every screen that shows them
    static constexpr uint16_t ATTACK = 0xEB8B;     // #E8735A
    static constexpr uint16_t DEFENSE = 0x7D5A;    // #7FA8D6
    static constexpr uint16_t HP = 0x7E2D;         // #7CC46A
    static constexpr uint16_t ENEMY_NAME = 0xF52F; // #F0A47E warm: the enemy's side
    static constexpr uint16_t PLAYER_LINE = TEXT_2; // cool: the player's side

    // The red cap of the physical Confirm button
    static constexpr uint16_t CONFIRM = 0xE309;     // #E0604E
    static constexpr uint16_t CONFIRM_DIM = 0x5964; // #5B2D25

    // Bars: a fill dark enough for a white label, a track of ~20% of it over the
    // background for the empty part, and a border at ~55%
    static constexpr uint16_t GOLD_BAR = 0x7AA4;        // #7E5627 progress toward a gold target
    static constexpr uint16_t GOLD_TRACK = 0x28E2;      // #291F14
    static constexpr uint16_t GOLD_BORDER = 0x49A3;     // #4E371C
    static constexpr uint16_t DONE_BAR = 0x5C69;        // #5D8E4F a gold target reached
    static constexpr uint16_t XP_BAR = 0x338D;          // #35716F teal, the player's colour
    static constexpr uint16_t XP_TRACK = 0x1924;        // #1B2422
    static constexpr uint16_t XP_BORDER = 0x2228;       // #264644
    static constexpr uint16_t HP_BAR = 0x64EA;          // #659D56
    static constexpr uint16_t HP_TRACK = 0x2163;        // #242D1D
    static constexpr uint16_t HP_BORDER = 0x42E6;       // #415E36
    static constexpr uint16_t ENEMY_HP_BAR = 0x91C6;    // #923B31
    static constexpr uint16_t ENEMY_HP_TRACK = 0x28C2;  // #2D1916
    static constexpr uint16_t ENEMY_HP_BORDER = 0x5944; // #592822
    static constexpr uint16_t ENEMY_ATK_BAR = 0xBAE9;   // #B95D4A
    static constexpr uint16_t ENEMY_ATK_TRACK = 0x3103; // #35201B
    static constexpr uint16_t ENEMY_ATK_BORDER = 0x69C5; // #6F3B2F
    static constexpr uint16_t INERT_BAR = FRAME;        // a full bar that does nothing

    // The enemy's stat card on the combat screen
    static constexpr uint16_t ENEMY_FRAME = 0xB227; // #B5473A
    static constexpr uint16_t ENEMY_BAND = 0x28C2;  // #2D1916 its title strip
}

#endif // UI_THEME_H
