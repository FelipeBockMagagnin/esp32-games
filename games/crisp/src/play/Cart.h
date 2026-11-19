#ifndef CART_H
#define CART_H

#include <stdint.h>

// How the five buttons reach a game, from library.json's `controls`
enum CartControls
{
    CONTROLS_TAP,    // OK only; every direction is free
    CONTROLS_ARROWS, // The directions and OK, but for the one that opens the menu
    CONTROLS_SLIDE   // Left and Right move the pointer the game follows, OK taps
};

enum CartKey
{
    CART_KEY_UP,
    CART_KEY_DOWN,
    CART_KEY_LEFT,
    CART_KEY_RIGHT
};

// A game in the firmware, in the order of crisp's own list. tools/carts.py writes the table
// from library.json.
struct Cart
{
    const char *slug;
    const char *author;
    uint32_t id; // FNV-1a of the slug: names the game's record in NVS
    uint8_t controls;
    uint8_t menuKey;
};

#ifdef __cplusplus
extern "C" {
#endif
extern const struct Cart CARTS[];
extern const uint16_t CART_COUNT;
#ifdef __cplusplus
}
#endif

#endif // CART_H
