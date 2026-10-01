#ifndef ITEM_CONFIG_H
#define ITEM_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"

enum class EquipSlot : uint8_t
{
    HELMET,
    BODY,
    PANTS,
    BOOTS,
    GLOVES,
    SWORD,
    SHIELD,
    RING,
    AMULET,
    COUNT
};

static constexpr uint8_t EQUIP_SLOT_COUNT = (uint8_t)EquipSlot::COUNT;

// An empty equipment slot, and the "no drop" result
static const int8_t NO_ITEM = -1;

static const char *const EQUIP_SLOT_NAMES[EQUIP_SLOT_COUNT] = {
    "Helmet", "Body", "Pants", "Boots", "Gloves", "Sword", "Shield", "Ring", "Amulet"};

// A duplicate drop levels an item up instead of stacking, adding this much of its
// base stats per level above 1
static const uint16_t ITEM_LEVEL_GROWTH_PERCENT = 15;
static const uint8_t ITEM_MAX_LEVEL = 99;

struct ItemDef
{
    const char *name; // Up to 11 chars to fit a list row title at text size 2
    EquipSlot slot;
    uint16_t attack;  // Base stats at level 1
    uint16_t defense;
    uint16_t maxHp;
    // Rare items feed back into the economy
    uint16_t goldProductionBonusPercent;
    uint16_t clickBonusPercent;
    // Exactly one of the two icon pointers is set, mirroring how Image draws both kinds.
    // Everything reuses already-converted art; new PNGs only need the pointer swapped here.
    const unsigned char *iconBitmap;
    const uint16_t *iconPixels;
    int16_t iconW;
    int16_t iconH;
};

#define ARMOR_ICON nullptr, image_armor_01b_pixels, 16, 16
#define SWORD_ICON nullptr, image_sword_02b_pixels, 16, 16
#define TRINKET_ICON image_device_key_retro_bits, nullptr, 16, 16
#define PLAIN_ICON image_cursor_black_white_bits, nullptr, 11, 16

// Names for the ITEMS table below, so drop tables read as names instead of numbers.
// The order here must match the table.
enum ItemId : uint8_t
{
    ITEM_CLOTH_HOOD, ITEM_IRON_HELM, ITEM_STEEL_HELM, ITEM_RUNE_HELM,
    ITEM_CLOTH_VEST, ITEM_IRON_MAIL, ITEM_STEEL_PLATE, ITEM_RUNE_PLATE,
    ITEM_CLOTH_PANTS, ITEM_IRON_LEGS, ITEM_STEEL_LEGS, ITEM_RUNE_LEGS,
    ITEM_WORN_BOOTS, ITEM_IRON_BOOTS, ITEM_STEEL_BOOTS, ITEM_RUNE_BOOTS,
    ITEM_RAG_GLOVES, ITEM_IRON_GLOVES, ITEM_STEEL_GRIPS, ITEM_RUNE_GRIPS,
    ITEM_RUSTY_SWORD, ITEM_IRON_SWORD, ITEM_STEEL_SWORD, ITEM_RUNE_BLADE,
    ITEM_WOOD_SHIELD, ITEM_IRON_SHIELD, ITEM_STEEL_GUARD, ITEM_RUNE_AEGIS,
    ITEM_TIN_RING, ITEM_GOLD_RING, ITEM_RUBY_RING, ITEM_MINER_RING,
    ITEM_BONE_CHARM, ITEM_JADE_AMULET, ITEM_GOLD_AMULET, ITEM_CORE_SIGIL
};

static constexpr ItemDef ITEMS[] = {
    //  name           slot                  atk  def  hp   prod%  click%  icon
    {"Cloth Hood",  EquipSlot::HELMET,    0,   2,   5,   0,     0,      ARMOR_ICON},
    {"Iron Helm",   EquipSlot::HELMET,    0,   6,   15,  0,     0,      ARMOR_ICON},
    {"Steel Helm",  EquipSlot::HELMET,    0,   18,  45,  0,     0,      ARMOR_ICON},
    {"Rune Helm",   EquipSlot::HELMET,    0,   54,  135, 0,     0,      ARMOR_ICON},

    {"Cloth Vest",  EquipSlot::BODY,      0,   4,   10,  0,     0,      ARMOR_ICON},
    {"Iron Mail",   EquipSlot::BODY,      0,   12,  30,  0,     0,      ARMOR_ICON},
    {"Steel Plate", EquipSlot::BODY,      0,   36,  90,  0,     0,      ARMOR_ICON},
    {"Rune Plate",  EquipSlot::BODY,      0,   108, 270, 0,     0,      ARMOR_ICON},

    {"Cloth Pants", EquipSlot::PANTS,     0,   3,   6,   0,     0,      ARMOR_ICON},
    {"Iron Legs",   EquipSlot::PANTS,     0,   9,   18,  0,     0,      ARMOR_ICON},
    {"Steel Legs",  EquipSlot::PANTS,     0,   27,  54,  0,     0,      ARMOR_ICON},
    {"Rune Legs",   EquipSlot::PANTS,     0,   81,  162, 0,     0,      ARMOR_ICON},

    {"Worn Boots",  EquipSlot::BOOTS,     0,   2,   4,   0,     0,      ARMOR_ICON},
    {"Iron Boots",  EquipSlot::BOOTS,     0,   6,   12,  0,     0,      ARMOR_ICON},
    {"Steel Boots", EquipSlot::BOOTS,     0,   18,  36,  0,     0,      ARMOR_ICON},
    {"Rune Boots",  EquipSlot::BOOTS,     0,   54,  108, 0,     0,      ARMOR_ICON},

    {"Rag Gloves",  EquipSlot::GLOVES,    1,   1,   0,   0,     0,      ARMOR_ICON},
    {"Iron Gloves", EquipSlot::GLOVES,    3,   3,   0,   0,     0,      ARMOR_ICON},
    {"Steel Grips", EquipSlot::GLOVES,    9,   9,   0,   0,     0,      ARMOR_ICON},
    {"Rune Grips",  EquipSlot::GLOVES,    27,  27,  0,   0,     0,      ARMOR_ICON},

    {"Rusty Sword", EquipSlot::SWORD,     5,   0,   0,   0,     0,      SWORD_ICON},
    {"Iron Sword",  EquipSlot::SWORD,     15,  0,   0,   0,     0,      SWORD_ICON},
    {"Steel Sword", EquipSlot::SWORD,     45,  0,   0,   0,     0,      SWORD_ICON},
    {"Rune Blade",  EquipSlot::SWORD,     135, 0,   0,   0,     0,      SWORD_ICON},

    {"Wood Shield", EquipSlot::SHIELD,    0,   5,   5,   0,     0,      ARMOR_ICON},
    {"Iron Shield", EquipSlot::SHIELD,    0,   15,  15,  0,     0,      ARMOR_ICON},
    {"Steel Guard", EquipSlot::SHIELD,    0,   45,  45,  0,     0,      ARMOR_ICON},
    {"Rune Aegis",  EquipSlot::SHIELD,    0,   135, 135, 0,     0,      ARMOR_ICON},

    {"Tin Ring",    EquipSlot::RING,      2,   0,   0,   0,     5,      TRINKET_ICON},
    {"Gold Ring",   EquipSlot::RING,      6,   0,   0,   0,     15,     TRINKET_ICON},
    {"Ruby Ring",   EquipSlot::RING,      18,  0,   0,   0,     40,     TRINKET_ICON},
    {"Miner Ring",  EquipSlot::RING,      54,  0,   0,   0,     100,    TRINKET_ICON},

    {"Bone Charm",  EquipSlot::AMULET,    0,   2,   0,   5,     0,      PLAIN_ICON},
    {"Jade Amulet", EquipSlot::AMULET,    0,   6,   0,   15,    0,      PLAIN_ICON},
    {"Gold Amulet", EquipSlot::AMULET,    0,   18,  0,   40,    0,      PLAIN_ICON},
    {"Core Sigil",  EquipSlot::AMULET,    0,   54,  0,   100,   0,      PLAIN_ICON},
};
static constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

#undef ARMOR_ICON
#undef SWORD_ICON
#undef TRINKET_ICON
#undef PLAIN_ICON

// Level 1 gives the base value; every level after adds ITEM_LEVEL_GROWTH_PERCENT of it
inline uint32_t scaleItemStat(uint16_t base, uint8_t level)
{
    if (base == 0 || level == 0)
    {
        return 0;
    }
    return (uint32_t)base * (100 + (uint32_t)(level - 1) * ITEM_LEVEL_GROWTH_PERCENT) / 100;
}

#endif // ITEM_CONFIG_H
