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

static const char *const STARTING_KIT[] = {"Dull Stick", "Cloth Vest"};

uint8_t Inventory::getStartingKitSize()
{
    return sizeof(STARTING_KIT) / sizeof(STARTING_KIT[0]);
}

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

// By tier, then table order, so a tool listed at the end of the table sits with its tier
int8_t Inventory::itemForSlotAt(EquipSlot slot, uint8_t index) const
{
    uint8_t seen = 0;
    for (uint8_t tier = 0; tier < ITEM_TIER_COUNT; tier++)
    {
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if (ITEMS[i].slot == slot && ITEMS[i].tier == tier && itemLevels[i] > 0)
            {
                if (seen == index)
                {
                    return (int8_t)i;
                }
                seen++;
            }
        }
    }
    return NO_ITEM;
}

Stats Inventory::getItemStats(uint8_t itemId) const
{
    Stats out = {0, 0, 0, 0};
    if (itemId >= ITEM_COUNT)
    {
        return out;
    }

    const ItemDef &def = ITEMS[itemId];
    uint8_t level = itemLevels[itemId];
    out.attack = scaleItemStat(def.attack, level);
    out.defense = scaleItemStat(def.defense, level);
    out.maxHp = scaleItemStat(def.maxHp, level);
    out.regen = scaleItemStat(def.regen, level);
    return out;
}

Stats Inventory::getEquippedStats() const
{
    Stats total = {0, 0, 0, 0};
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
        total.regen += item.regen;
    }
    return total;
}

// Summed over everything worn, uncapped: each consumer applies its own cap
uint32_t Inventory::getPerkPercent(ItemPerk perk) const
{
    uint32_t total = 0;
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        if (equipped[slot] == NO_ITEM)
        {
            continue;
        }
        total += getItemPerk((uint8_t)equipped[slot], perk);
    }
    return total;
}

uint32_t Inventory::getItemPerk(uint8_t itemId, ItemPerk perk) const
{
    if (itemId >= ITEM_COUNT)
    {
        return 0;
    }
    uint32_t total = 0;
    for (const PerkValue &p : ITEMS[itemId].perks)
    {
        if (p.kind == perk)
        {
            bool flat = perk == ItemPerk::RESPAWN_CUT || perk == ItemPerk::DROP;
            total += flat ? p.value : scaleItemStat(p.value, itemLevels[itemId]);
        }
    }
    return total;
}

uint32_t Inventory::getRelicPercent(RelicEffect effect) const
{
    int8_t worn = equipped[(uint8_t)EquipSlot::RELIC];
    if (worn == NO_ITEM || ITEMS[(uint8_t)worn].relic != effect)
    {
        return 0;
    }
    return scaleItemStat(ITEMS[(uint8_t)worn].relicPercent, itemLevels[(uint8_t)worn]);
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
        // Drop anything that points at an item the player does not own, or at one from another slot
        int8_t id = in.equipped[i];
        bool valid = id >= 0 && id < (int8_t)ITEM_COUNT && itemLevels[id] > 0 && (uint8_t)ITEMS[id].slot == i;
        equipped[i] = valid ? id : NO_ITEM;
    }
    revision++;
}
