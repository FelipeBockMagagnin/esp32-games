#ifndef GFX_THEME_H
#define GFX_THEME_H

#include <Arduino.h>

// Every colour on the panel is one of these slots, looked up in the active palette (THEMES in
// Themes.h) at draw time, so a theme change is only a repaint. A new slot needs a column in
// every theme.
static constexpr uint8_t BG = 0;        // the screen behind the cards
static constexpr uint8_t CARD = 1;      // cards and panels
static constexpr uint8_t CARD_EDGE = 2; // a card's soft outline
static constexpr uint8_t INK = 3;       // text
static constexpr uint8_t INK_SOFT = 4;  // secondary text
static constexpr uint8_t INK_FAINT = 5; // hints, unavailable, silhouettes
static constexpr uint8_t WALL_TOP = 6;  // the room's wall, a gradient
static constexpr uint8_t WALL_BOTTOM = 7;
static constexpr uint8_t FLOOR = 8;     // the floorboards
static constexpr uint8_t FLOOR_LINE = 9;// their joints
static constexpr uint8_t RUG = 10;      // the round rug under the pet
static constexpr uint8_t SKY_DAY = 11;  // the window by day
static constexpr uint8_t SKY_NIGHT = 12;// and by night
static constexpr uint8_t NIGHT_TINT = 13;// what the room fades toward with the lamp off
static constexpr uint8_t BODY_A = 14;   // a glyph's body: each form picks one
static constexpr uint8_t BODY_B = 15;
static constexpr uint8_t BODY_C = 16;
static constexpr uint8_t BODY_D = 17;
static constexpr uint8_t BODY_E = 18;
static constexpr uint8_t EYE = 19;      // eyes and mouths
static constexpr uint8_t SHINE = 20;    // eye shine, highlights
static constexpr uint8_t BLUSH = 21;    // cheeks, hearts, food
static constexpr uint8_t GOLD = 22;     // stars, the sun, rewards, joy
static constexpr uint8_t WATER = 23;    // rest, tears, rain, medicine
static constexpr uint8_t LEAF = 24;     // plants, tidy
static constexpr uint8_t ALERT = 25;    // a soft coral: low needs, sickness
static constexpr uint8_t ACCENT = 26;   // the selection
static constexpr uint8_t ACCENT_INK = 27;// text on the selection
static constexpr uint8_t SHADOW = 28;   // shadows under things
static constexpr uint8_t CONFIRM = 29;  // the red cap of the physical Confirm button
static constexpr uint8_t COLOR_SLOT_COUNT = 30;

namespace Theme
{
    extern const uint16_t *activePalette;

    inline uint16_t rgb(uint8_t slot) { return activePalette[slot < COLOR_SLOT_COUNT ? slot : INK]; }

    uint8_t getActive();
    // Out-of-range indices fall back to the first theme
    void setActive(uint8_t index);
}

#endif // GFX_THEME_H
