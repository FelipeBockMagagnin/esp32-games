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

// Item tiers, worst to best within a slot. The name color follows the familiar RPG
// rarity scheme, so a better drop stands out in a list before its stats are read.
static const uint8_t ITEM_TIER_COUNT = 4;
static const char *const ITEM_TIER_NAMES[ITEM_TIER_COUNT] = {"Common", "Uncommon", "Rare", "Epic"};
static const uint16_t ITEM_TIER_COLORS[ITEM_TIER_COUNT] = {0xD69A, 0x7F0F, 0x655F, 0xD3DF};

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
    uint8_t tier;     // 0-3, from the slot's first item to its best; drives the name color
    // Exactly one of the two icon pointers is set, mirroring how Image draws both kinds.
    // New art only needs the pointer swapped here.
    const unsigned char *iconBitmap;
    const uint16_t *iconPixels;
    int16_t iconW;
    int16_t iconH;
};

// Every item has its own 16x16 RGB565 icon
#define ICON(pixels) nullptr, pixels, 16, 16

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
    //  name           slot                  atk  def  hp   prod%  click%  tier  icon
    {"Cloth Hood",  EquipSlot::HELMET,    0,   2,   5,   0,     0,      0,     ICON(image_item_cloth_hood_pixels)},
    {"Iron Helm",   EquipSlot::HELMET,    0,   6,   15,  0,     0,      1,     ICON(image_item_iron_helm_pixels)},
    {"Steel Helm",  EquipSlot::HELMET,    0,   18,  45,  0,     0,      2,     ICON(image_item_steel_helm_pixels)},
    {"Rune Helm",   EquipSlot::HELMET,    0,   54,  135, 0,     0,      3,     ICON(image_item_rune_helm_pixels)},

    {"Cloth Vest",  EquipSlot::BODY,      0,   4,   10,  0,     0,      0,     ICON(image_item_cloth_vest_pixels)},
    {"Iron Mail",   EquipSlot::BODY,      0,   12,  30,  0,     0,      1,     ICON(image_armor_01b_pixels)},
    {"Steel Plate", EquipSlot::BODY,      0,   36,  90,  0,     0,      2,     ICON(image_item_steel_plate_pixels)},
    {"Rune Plate",  EquipSlot::BODY,      0,   108, 270, 0,     0,      3,     ICON(image_item_rune_plate_pixels)},

    {"Cloth Pants", EquipSlot::PANTS,     0,   3,   6,   0,     0,      0,     ICON(image_item_cloth_pants_pixels)},
    {"Iron Legs",   EquipSlot::PANTS,     0,   9,   18,  0,     0,      1,     ICON(image_item_iron_legs_pixels)},
    {"Steel Legs",  EquipSlot::PANTS,     0,   27,  54,  0,     0,      2,     ICON(image_item_steel_legs_pixels)},
    {"Rune Legs",   EquipSlot::PANTS,     0,   81,  162, 0,     0,      3,     ICON(image_item_rune_legs_pixels)},

    {"Worn Boots",  EquipSlot::BOOTS,     0,   2,   4,   0,     0,      0,     ICON(image_item_worn_boots_pixels)},
    {"Iron Boots",  EquipSlot::BOOTS,     0,   6,   12,  0,     0,      1,     ICON(image_item_iron_boots_pixels)},
    {"Steel Boots", EquipSlot::BOOTS,     0,   18,  36,  0,     0,      2,     ICON(image_item_steel_boots_pixels)},
    {"Rune Boots",  EquipSlot::BOOTS,     0,   54,  108, 0,     0,      3,     ICON(image_item_rune_boots_pixels)},

    {"Rag Gloves",  EquipSlot::GLOVES,    1,   1,   0,   0,     0,      0,     ICON(image_item_rag_gloves_pixels)},
    {"Iron Gloves", EquipSlot::GLOVES,    3,   3,   0,   0,     0,      1,     ICON(image_item_iron_gloves_pixels)},
    {"Steel Grips", EquipSlot::GLOVES,    9,   9,   0,   0,     0,      2,     ICON(image_item_steel_grips_pixels)},
    {"Rune Grips",  EquipSlot::GLOVES,    27,  27,  0,   0,     0,      3,     ICON(image_item_rune_grips_pixels)},

    {"Rusty Sword", EquipSlot::SWORD,     5,   0,   0,   0,     0,      0,     ICON(image_item_rusty_sword_pixels)},
    {"Iron Sword",  EquipSlot::SWORD,     15,  0,   0,   0,     0,      1,     ICON(image_sword_02b_pixels)},
    {"Steel Sword", EquipSlot::SWORD,     45,  0,   0,   0,     0,      2,     ICON(image_item_steel_sword_pixels)},
    {"Rune Blade",  EquipSlot::SWORD,     135, 0,   0,   0,     0,      3,     ICON(image_item_rune_blade_pixels)},

    {"Wood Shield", EquipSlot::SHIELD,    0,   5,   5,   0,     0,      0,     ICON(image_item_wood_shield_pixels)},
    {"Iron Shield", EquipSlot::SHIELD,    0,   15,  15,  0,     0,      1,     ICON(image_item_iron_shield_pixels)},
    {"Steel Guard", EquipSlot::SHIELD,    0,   45,  45,  0,     0,      2,     ICON(image_item_steel_guard_pixels)},
    {"Rune Aegis",  EquipSlot::SHIELD,    0,   135, 135, 0,     0,      3,     ICON(image_item_rune_aegis_pixels)},

    {"Tin Ring",    EquipSlot::RING,      2,   0,   0,   0,     5,      0,     ICON(image_item_tin_ring_pixels)},
    {"Gold Ring",   EquipSlot::RING,      6,   0,   0,   0,     15,      1,     ICON(image_item_gold_ring_pixels)},
    {"Ruby Ring",   EquipSlot::RING,      18,  0,   0,   0,     40,      2,     ICON(image_item_ruby_ring_pixels)},
    {"Miner Ring",  EquipSlot::RING,      54,  0,   0,   0,     100,      3,     ICON(image_item_miner_ring_pixels)},

    {"Bone Charm",  EquipSlot::AMULET,    0,   2,   0,   5,     0,      0,     ICON(image_item_bone_charm_pixels)},
    {"Jade Amulet", EquipSlot::AMULET,    0,   6,   0,   15,    0,      1,     ICON(image_item_jade_amulet_pixels)},
    {"Gold Amulet", EquipSlot::AMULET,    0,   18,  0,   40,    0,      2,     ICON(image_item_gold_amulet_pixels)},
    {"Core Sigil",  EquipSlot::AMULET,    0,   54,  0,   100,   0,      3,     ICON(image_item_core_sigil_pixels)},
};
static constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

#undef ICON

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
