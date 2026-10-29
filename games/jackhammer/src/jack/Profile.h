#ifndef JACK_PROFILE_H
#define JACK_PROFILE_H

#include <stdint.h>
#include "Config.h"

// Everything that outlives a round: the coins, the best score and the shop. POD, since the
// save writes it raw.
struct Profile
{
    uint32_t coins;
    uint32_t best;
    uint32_t rounds;
    uint8_t ownedStyles; // Bit per STYLES row
    uint8_t ownedSongs;  // Bit per SONGS row
    uint8_t style;
    uint8_t song;
};

enum class Shelf : uint8_t
{
    STYLES,
    SONGS
};

// What a new player owns: the free rows
void newProfile(Profile &p);
// Fixes indices a corrupted or hand-edited save could hold
void sanitize(Profile &p);

const ShopItem &shopItem(Shelf shelf, uint8_t index);
uint8_t shelfSize(Shelf shelf);
bool owns(const Profile &p, Shelf shelf, uint8_t index);
bool isEquipped(const Profile &p, Shelf shelf, uint8_t index);
// Buys it if needed and affordable, then wears or plays it; false when it cannot be paid
bool buyOrEquip(Profile &p, Shelf shelf, uint8_t index);

// Banks a finished round's coins and counts it; true when it set a new best
bool bankRound(Profile &p, uint32_t points, uint32_t earned);
bool pay(Profile &p, uint32_t price);

#endif // JACK_PROFILE_H
