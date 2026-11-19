#include "Cart.h"
#include "cglp.h"

// CartTable.inc is written by tools/carts.py from library.json, into the build folder:
// CART(slug, author, id, controls, menuKey, addFunction) once per game, in list order
#define CART(slug, author, id, controls, menuKey, add) void add(void);
#include "CartTable.inc"
#undef CART

const struct Cart CARTS[] = {
#define CART(slug, author, id, controls, menuKey, add) {slug, author, id, controls, menuKey},
#include "CartTable.inc"
#undef CART
};
const uint16_t CART_COUNT = sizeof(CARTS) / sizeof(CARTS[0]);
_Static_assert(sizeof(CARTS) / sizeof(CARTS[0]) <= MAX_GAME_COUNT, "more games than MAX_GAME_COUNT");

void addGames() {
#define CART(slug, author, id, controls, menuKey, add) add();
#include "CartTable.inc"
#undef CART
}
