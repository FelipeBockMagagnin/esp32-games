#ifndef CRAWL_ITEMS_H
#define CRAWL_ITEMS_H

#include <stddef.h>
#include "Config.h"
#include "Rng.h"

// A piece of gear: one stat, its value in the slot's unit (SlotDef). Value 0 is no item.
struct Item
{
    uint8_t slot;
    uint8_t tier;
    uint8_t series; // Which look of the tier, and which noun
    uint8_t pad;
    uint16_t value;

    bool empty() const { return value == 0; }
};

Item rollItem(Rng &rng, uint8_t floor, uint8_t tierBonus);
Item starterWeapon();
// "Grim Longsword"
void itemName(const Item &item, char *out, size_t size);
// "+12 dmg"
void itemStat(const Item &item, char *out, size_t size);

enum class Pick : uint8_t
{
    EQUIPPED,  // Better than what was worn, which went to the bag (or back to the floor if full)
    BAGGED,
    BAG_FULL   // Not better and no room: it stays where it lies
};

struct Inventory
{
    Item equipped[SLOT_COUNT];
    Item bag[BAG_SIZE];
    uint8_t hpPotions;
    uint8_t mpPotions;

    void clear();
    uint8_t bagCount() const;
    // Equips the item only if it beats the one in its slot. `displaced` is what has to go back
    // on the floor: the old item when the bag had no room for it, the new one on BAG_FULL.
    Pick pickUp(const Item &item, Item &displaced);
    // Swaps a bag item with the one worn in its slot
    void equipFromBag(uint8_t index);
    void dropFromBag(uint8_t index);
    uint16_t stat(Slot slot) const { return equipped[slot].value; }

private:
    bool stash(const Item &item);
};

#endif // CRAWL_ITEMS_H
