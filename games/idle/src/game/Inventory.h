#ifndef INVENTORY_H
#define INVENTORY_H

#include <Arduino.h>
#include "ItemConfig.h"
#include "Stats.h"

// Owns which items the player has found and what is equipped. A duplicate drop levels
// the item up rather than stacking, so ownership is one byte per item.
class Inventory
{
public:
    struct Snapshot
    {
        uint8_t itemLevels[ITEM_COUNT];
        int8_t equipped[EQUIP_SLOT_COUNT];
    };

    Inventory();

    // First drop of an item makes it level 1, a repeat raises its level.
    // Returns true when the item was new, so the UI can say so.
    bool addDrop(uint8_t itemId);

    bool equip(uint8_t itemId);
    void unequip(EquipSlot slot);

    uint8_t getItemLevel(uint8_t itemId) const;
    bool isOwned(uint8_t itemId) const { return getItemLevel(itemId) > 0; }
    int8_t getEquipped(EquipSlot slot) const;

    // Walking the owned items of one slot, for the equip list
    uint8_t countItemsForSlot(EquipSlot slot) const;
    int8_t itemForSlotAt(EquipSlot slot, uint8_t index) const;

    // Stats of a single owned item at its current level
    Stats getItemStats(uint8_t itemId) const;

    Stats getEquippedStats() const;
    uint32_t getGoldProductionBonusPercent() const;
    uint32_t getClickBonusPercent() const;

    // Bumped on every change so screens and combat can notice without polling each item
    uint32_t getRevision() const { return revision; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    uint8_t itemLevels[ITEM_COUNT];
    int8_t equipped[EQUIP_SLOT_COUNT];
    uint32_t revision;
};

#endif // INVENTORY_H
