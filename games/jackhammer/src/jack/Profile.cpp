#include "Profile.h"
#include <string.h>

void newProfile(Profile &p)
{
    memset(&p, 0, sizeof(p));
    for (uint8_t i = 0; i < STYLE_COUNT; i++)
    {
        if (STYLES[i].price == 0)
        {
            p.ownedStyles |= 1 << i;
        }
    }
    for (uint8_t i = 0; i < SONG_COUNT; i++)
    {
        if (SONGS[i].price == 0)
        {
            p.ownedSongs |= 1 << i;
        }
    }
}

void sanitize(Profile &p)
{
    if (p.style >= STYLE_COUNT || !owns(p, Shelf::STYLES, p.style))
    {
        p.style = 0;
    }
    if (p.song >= SONG_COUNT || !owns(p, Shelf::SONGS, p.song))
    {
        p.song = 0;
    }
}

const ShopItem &shopItem(Shelf shelf, uint8_t index)
{
    return shelf == Shelf::STYLES ? STYLES[index] : SONGS[index];
}

uint8_t shelfSize(Shelf shelf)
{
    return shelf == Shelf::STYLES ? STYLE_COUNT : SONG_COUNT;
}

bool owns(const Profile &p, Shelf shelf, uint8_t index)
{
    uint8_t bits = shelf == Shelf::STYLES ? p.ownedStyles : p.ownedSongs;
    return index < shelfSize(shelf) && (bits & (1 << index)) != 0;
}

bool isEquipped(const Profile &p, Shelf shelf, uint8_t index)
{
    return (shelf == Shelf::STYLES ? p.style : p.song) == index;
}

bool buyOrEquip(Profile &p, Shelf shelf, uint8_t index)
{
    if (index >= shelfSize(shelf))
    {
        return false;
    }
    if (!owns(p, shelf, index))
    {
        if (!pay(p, shopItem(shelf, index).price))
        {
            return false;
        }
        (shelf == Shelf::STYLES ? p.ownedStyles : p.ownedSongs) |= 1 << index;
    }
    (shelf == Shelf::STYLES ? p.style : p.song) = index;
    return true;
}

bool bankRound(Profile &p, uint32_t points, uint32_t earned)
{
    p.coins = p.coins + earned >= p.coins ? p.coins + earned : UINT32_MAX;
    p.rounds++;
    if (points > p.best)
    {
        p.best = points;
        return true;
    }
    return false;
}

bool pay(Profile &p, uint32_t price)
{
    if (p.coins < price)
    {
        return false;
    }
    p.coins -= price;
    return true;
}
