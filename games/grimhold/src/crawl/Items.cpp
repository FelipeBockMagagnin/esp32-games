#include "Items.h"
#include <stdio.h>
#include <string.h>

Item rollItem(Rng &rng, uint8_t floor, uint8_t tierBonus)
{
    Item item = {};
    item.slot = (uint8_t)rng.below(SLOT_COUNT);
    const SlotDef &def = SLOTS[item.slot];
    // Mostly the floor's tier, now and then one above or below
    int16_t tier = (floor - 1) / 3 + tierBonus + (int16_t)rng.range(-1, 1) * (rng.below(3) == 0);
    tier = tier < 0 ? 0 : tier >= TIER_COUNT ? TIER_COUNT - 1 : tier;
    item.tier = (uint8_t)tier;
    item.series = (uint8_t)rng.below(def.series);
    uint32_t value = def.base + def.perTier * item.tier + def.perFloor * (floor - 1) / 10;
    // +-15%, at least one either way when the value allows
    uint32_t spread = value * 15 / 100 + 1;
    value = value + rng.below(2 * spread + 1) - spread;
    item.value = (uint16_t)(value < 1 ? 1 : value);
    return item;
}

Item starterWeapon()
{
    Item item = {};
    item.slot = SLOT_WEAPON;
    item.value = SLOTS[SLOT_WEAPON].base - 2;
    return item;
}

void itemName(const Item &item, char *out, size_t size)
{
    const SlotDef &def = SLOTS[item.slot];
    snprintf(out, size, "%s %s", TIER_NAMES[item.tier], def.names[item.series % def.series]);
}

void itemStat(const Item &item, char *out, size_t size)
{
    if (item.slot == SLOT_RING)
    {
        snprintf(out, size, "+%u.%u mana", item.value / 10, item.value % 10);
        return;
    }
    const char *stat = SLOTS[item.slot].stat;
    snprintf(out, size, "+%u%s%s", item.value, stat[0] == '%' ? "" : " ", stat);
}

void Inventory::clear()
{
    memset(this, 0, sizeof(*this));
}

uint8_t Inventory::bagCount() const
{
    uint8_t count = 0;
    for (const Item &item : bag)
    {
        count += !item.empty();
    }
    return count;
}

bool Inventory::stash(const Item &item)
{
    for (Item &slot : bag)
    {
        if (slot.empty())
        {
            slot = item;
            return true;
        }
    }
    return false;
}

Pick Inventory::pickUp(const Item &item, Item &displaced)
{
    displaced = Item{};
    Item &worn = equipped[item.slot];
    if (item.value > worn.value)
    {
        if (!worn.empty() && !stash(worn))
        {
            displaced = worn;
        }
        worn = item;
        return Pick::EQUIPPED;
    }
    if (stash(item))
    {
        return Pick::BAGGED;
    }
    displaced = item;
    return Pick::BAG_FULL;
}

void Inventory::equipFromBag(uint8_t index)
{
    if (index >= BAG_SIZE || bag[index].empty())
    {
        return;
    }
    Item item = bag[index];
    bag[index] = equipped[item.slot];
    equipped[item.slot] = item;
}

void Inventory::dropFromBag(uint8_t index)
{
    if (index < BAG_SIZE)
    {
        bag[index] = Item{};
    }
}
