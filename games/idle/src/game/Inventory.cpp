#include "Inventory.h"

// Looked up by name so the starting kit survives any reordering of the ITEMS table
static int8_t findItem(const char *name)
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (strcmp(ITEMS[i].name, name) == 0)
        {
            return (int8_t)i;
        }
    }
    return NO_ITEM;
}

static const char *const STARTING_KIT[] = {"Rusty Sword", "Cloth Vest"};

Inventory::Inventory()
    : revision(0)
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        itemLevels[i] = 0;
    }
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; i++)
    {
        equipped[i] = NO_ITEM;
    }

    for (uint8_t i = 0; i < sizeof(STARTING_KIT) / sizeof(STARTING_KIT[0]); i++)
    {
        int8_t id = findItem(STARTING_KIT[i]);
        if (id != NO_ITEM)
        {
            itemLevels[id] = 1;
            equip((uint8_t)id);
        }
    }
}

bool Inventory::addDrop(uint8_t itemId)
{
    if (itemId >= ITEM_COUNT)
    {
        return false;
    }

    revision++;

    if (itemLevels[itemId] == 0)
    {
        itemLevels[itemId] = 1;
        return true;
    }

    if (itemLevels[itemId] < ITEM_MAX_LEVEL)
    {
        itemLevels[itemId]++;
    }
    return false;
}

bool Inventory::equip(uint8_t itemId)
{
    if (itemId >= ITEM_COUNT || itemLevels[itemId] == 0)
    {
        return false;
    }

    uint8_t slot = (uint8_t)ITEMS[itemId].slot;
    if (equipped[slot] == (int8_t)itemId)
    {
        return false;
    }

    equipped[slot] = (int8_t)itemId;
    revision++;
    return true;
}

void Inventory::unequip(EquipSlot slot)
{
    uint8_t index = (uint8_t)slot;
    if (index < EQUIP_SLOT_COUNT && equipped[index] != NO_ITEM)
    {
        equipped[index] = NO_ITEM;
        revision++;
    }
}

uint8_t Inventory::getItemLevel(uint8_t itemId) const
{
    return itemId < ITEM_COUNT ? itemLevels[itemId] : 0;
}

int8_t Inventory::getEquipped(EquipSlot slot) const
{
    uint8_t index = (uint8_t)slot;
    return index < EQUIP_SLOT_COUNT ? equipped[index] : NO_ITEM;
}

uint8_t Inventory::countOwnedItems() const
{
    uint8_t total = 0;
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (itemLevels[i] > 0)
        {
            total++;
        }
    }
    return total;
}

uint8_t Inventory::getHighestItemLevel() const
{
    uint8_t best = 0;
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (itemLevels[i] > best)
        {
            best = itemLevels[i];
        }
    }
    return best;
}

uint8_t Inventory::countItemsForSlot(EquipSlot slot) const
{
    uint8_t total = 0;
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (ITEMS[i].slot == slot && itemLevels[i] > 0)
        {
            total++;
        }
    }
    return total;
}

int8_t Inventory::itemForSlotAt(EquipSlot slot, uint8_t index) const
{
    uint8_t seen = 0;
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (ITEMS[i].slot == slot && itemLevels[i] > 0)
        {
            if (seen == index)
            {
                return (int8_t)i;
            }
            seen++;
        }
    }
    return NO_ITEM;
}

Stats Inventory::getItemStats(uint8_t itemId) const
{
    Stats out = {0, 0, 0};
    if (itemId >= ITEM_COUNT)
    {
        return out;
    }

    const ItemDef &def = ITEMS[itemId];
    uint8_t level = itemLevels[itemId];
    out.attack = scaleItemStat(def.attack, level);
    out.defense = scaleItemStat(def.defense, level);
    out.maxHp = scaleItemStat(def.maxHp, level);
    return out;
}

Stats Inventory::getEquippedStats() const
{
    Stats total = {0, 0, 0};
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        if (equipped[slot] == NO_ITEM)
        {
            continue;
        }
        Stats item = getItemStats((uint8_t)equipped[slot]);
        total.attack += item.attack;
        total.defense += item.defense;
        total.maxHp += item.maxHp;
    }
    return total;
}

uint32_t Inventory::getGoldProductionBonusPercent() const
{
    uint32_t total = 0;
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        if (equipped[slot] != NO_ITEM)
        {
            uint8_t id = (uint8_t)equipped[slot];
            total += scaleItemStat(ITEMS[id].goldProductionBonusPercent, itemLevels[id]);
        }
    }
    return total;
}

uint32_t Inventory::getClickBonusPercent() const
{
    uint32_t total = 0;
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        if (equipped[slot] != NO_ITEM)
        {
            uint8_t id = (uint8_t)equipped[slot];
            total += scaleItemStat(ITEMS[id].clickBonusPercent, itemLevels[id]);
        }
    }
    return total;
}

void Inventory::save(Snapshot &out) const
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        out.itemLevels[i] = itemLevels[i];
    }
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; i++)
    {
        out.equipped[i] = equipped[i];
    }
}

void Inventory::load(const Snapshot &in)
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        itemLevels[i] = in.itemLevels[i];
    }
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; i++)
    {
        // Drop anything that points at an item the player does not actually own
        int8_t id = in.equipped[i];
        equipped[i] = (id >= 0 && id < (int8_t)ITEM_COUNT && itemLevels[id] > 0) ? id : NO_ITEM;
    }
    revision++;
}
