#ifndef ITEM_CONFIG_H
#define ITEM_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"
#include "../assets/Sprites.h"

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
    RELIC,
    COUNT
};

static constexpr uint8_t EQUIP_SLOT_COUNT = (uint8_t)EquipSlot::COUNT;

// An empty equipment slot, and the "no drop" result
static const int8_t NO_ITEM = -1;

static const char *const EQUIP_SLOT_NAMES[EQUIP_SLOT_COUNT] = {
    "Helmet", "Body", "Pants", "Boots", "Gloves", "Sword", "Shield", "Ring", "Amulet", "Relic"};

// Item tiers, worst to best within a slot. The name color follows the familiar RPG
// rarity scheme, so a better drop stands out in a list before its stats are read.
// Mythic gear drops only past the Molten Core, in the zones Deep Delver opens; relics are
// a tier of their own, outside the gear ladder.
static const uint8_t ITEM_TIER_COUNT = 6;
static const uint8_t MYTHIC_TIER = 4;
static const uint8_t RELIC_TIER = 5;
static const char *const ITEM_TIER_NAMES[ITEM_TIER_COUNT] = {"Common", "Uncommon", "Rare", "Epic", "Mythic", "Relic"};
static const uint16_t ITEM_TIER_COLORS[ITEM_TIER_COUNT] = {0xBD52, 0x8E4D, 0x7D5A, 0xC45B, 0xF443, 0xF5A8}; // Theme TEXT_2, GOOD, DEFENSE, EPIC, MYTHIC, GOLD

// What the worn relic does, as relicPercent of it scaled by the relic's level like a stat.
// Relics drop only once the Relic Sense talent has a level per relic, in table order.
enum class RelicEffect : uint8_t
{
    NONE,
    UPGRADE_DISCOUNT, // Gold upgrades cost less
    COOLDOWN_CUT,     // Strike and guard come back sooner
    AWARD_BOOST,      // The awards' gold production bonus grows by this share
    HP_REGEN,         // Tenths of a percent of max HP regained per second
    COUNT
};

// What an item adds besides combat stats. Every perk but the respawn cut and the drop
// chance scales with the item's level like a stat; a levelled drop perk made duplicates
// come faster the more of them were found, maxing whole sets within the first run. The
// summed caps below keep stacking them from breaking a system.
enum class ItemPerk : uint8_t
{
    NONE,
    GOLD,             // Gold production
    CLICK,            // Gold per click
    DROP,             // Scales every drop's chance, not levelled
    RESPAWN_CUT,      // Flat cut to the wait between enemies, not levelled
    KILL_GOLD,        // Gold per kill
    DOUBLE_MINE,      // Chance a click mines twice
    MINE_XP,          // Chance a click gives double mining XP
    UPGRADE_DISCOUNT, // Taken off gold upgrade prices, with the Trader Coin's
    COUNT
};

struct PerkValue
{
    ItemPerk kind;
    uint16_t value;
};

static const uint8_t ITEM_PERK_SLOTS = 3;

// A duplicate drop levels an item up instead of stacking, adding this many thousandths of
// its base stats per level above 1. The cap keeps a maxed item (+148.5%) below the next
// tier's base (3x), so farming duplicates never replaces moving on to a new zone. A
// hundred small steps make finding an item quick and maxing it a goal across ascensions.
static const uint16_t ITEM_LEVEL_GROWTH_PERMILLE = 15;
static const uint8_t ITEM_MAX_LEVEL = 100;

struct ItemDef
{
    const char *name; // Up to 11 chars to fit a list row title at text size 2
    EquipSlot slot;
    uint16_t attack;  // Base stats at level 1
    uint16_t defense;
    uint16_t maxHp;
    // Tenths of HP regained per second, in a fight and between enemies. A few pieces, one
    // per tier, carry it at ~0.1-0.3% of the max HP the player has when that tier is worn,
    // so it shaves a tenth or so off the incoming damage rather than out-healing it.
    uint16_t regen;
    uint8_t tier;     // Drives the name color; see ITEM_TIER_NAMES
    // Exactly one of the two icon pointers is set, mirroring how Image draws both kinds.
    // New art only needs the pointer swapped here.
    const unsigned char *iconBitmap;
    const uint16_t *iconPixels;
    int16_t iconW;
    int16_t iconH;
    PerkValue perks[ITEM_PERK_SLOTS]; // At level 1; unused slots zero-initialize to NONE
    RelicEffect relic;
    uint16_t relicPercent; // At level 1; levels scale it like a stat
    // A tool trades about 40% of its slot's combat stats for perks: worn to mine or to farm
    // a zone already beaten, never to open one. Tools are no part of a gear set.
    bool tool;
};

// Caps on the summed perks of everything worn (the respawn cut's also counts the awards')
static const uint8_t MAX_RESPAWN_CUT_PERCENT = 50;
static const uint8_t MAX_DOUBLE_MINE_PERCENT = 50;
static const uint8_t MAX_MINE_XP_PERCENT = 100;
static const uint8_t MAX_UPGRADE_DISCOUNT_PERCENT = 50;

// Every item has its own 16x16 RGB565 icon
#define ICON(pixels) nullptr, pixels, 16, 16
#define P(kind, value) {ItemPerk::kind, value}

// Names for the ITEMS table below, so drop tables read as names instead of numbers.
// The order here must match the table.
enum ItemId : uint8_t
{
    ITEM_LEAF_HAT, ITEM_LEATHER_CAP, ITEM_STEEL_HELM, ITEM_RUNE_HELM,
    ITEM_CLOTH_VEST, ITEM_IRON_MAIL, ITEM_STEEL_PLATE, ITEM_RUNE_PLATE,
    ITEM_FIG_LEAF, ITEM_LOINCLOTH, ITEM_STEEL_LEGS, ITEM_RUNE_LEGS,
    ITEM_WORN_BOOTS, ITEM_IRON_BOOTS, ITEM_STEEL_BOOTS, ITEM_RUNE_BOOTS,
    ITEM_RAG_GLOVES, ITEM_IRON_GLOVES, ITEM_STEEL_GRIPS, ITEM_RUNE_GRIPS,
    ITEM_DULL_STICK, ITEM_GOOD_STICK, ITEM_GREAT_STICK, ITEM_RUNE_BLADE,
    ITEM_WOOD_SHIELD, ITEM_IRON_SHIELD, ITEM_STEEL_GUARD, ITEM_RUNE_AEGIS,
    ITEM_TIN_RING, ITEM_GOLD_RING, ITEM_RUBY_RING, ITEM_MINER_RING,
    ITEM_WILSON, ITEM_JADE_AMULET, ITEM_GOLD_AMULET, ITEM_CORE_SIGIL,
    ITEM_EMBER_HELM, ITEM_EMBER_MAIL, ITEM_EMBER_LEGS, ITEM_EMBER_BOOTS, ITEM_EMBER_GRIPS,
    ITEM_EMBER_BLADE, ITEM_EMBER_GUARD, ITEM_EMBER_RING, ITEM_EMBER_CHARM,
    ITEM_TRADER_COIN, ITEM_TIME_CANDLE, ITEM_ELDER_BONE, ITEM_GOLEM_HEART,
    ITEM_RUSTY_PICK, ITEM_LUCKY_BOOTS, ITEM_IRON_PICK, ITEM_MINER_HELM, ITEM_LOOT_GLOVES,
    ITEM_STEEL_PICK, ITEM_GRAVE_HOOD, ITEM_COIN_RING, ITEM_RUNE_PICK, ITEM_SCOUT_BOOTS,
    ITEM_GILT_SHIELD, ITEM_VOID_PICK, ITEM_STAR_CROWN
};

static constexpr ItemDef ITEMS[] = {
    //  name           slot               atk  def  hp   rgn  tier  icon                                  perks
    {"Leaf Hat",    EquipSlot::HELMET,    0,   2,   5,   3,   0,    ICON(image_item_leaf_hat_pixels)},
    {"Leather Cap", EquipSlot::HELMET,    0,   6,   15,  0,   1,    ICON(image_item_leather_cap_pixels)},
    {"Steel Helm",  EquipSlot::HELMET,    0,   18,  45,  0,   2,    ICON(image_item_steel_helm_pixels)},
    {"Rune Helm",   EquipSlot::HELMET,    0,   54,  135, 81,  3,    ICON(image_item_rune_helm_pixels)},

    {"Cloth Vest",  EquipSlot::BODY,      0,   4,   10,  0,   0,    ICON(image_item_cloth_vest_pixels)},
    {"Iron Mail",   EquipSlot::BODY,      0,   12,  30,  0,   1,    ICON(image_armor_01b_pixels)},
    {"Steel Plate", EquipSlot::BODY,      0,   36,  90,  27,  2,    ICON(image_item_steel_plate_pixels)},
    {"Rune Plate",  EquipSlot::BODY,      0,   108, 270, 0,   3,    ICON(image_item_rune_plate_pixels)},

    {"Fig Leaf",    EquipSlot::PANTS,     0,   3,   6,   0,   0,    ICON(image_item_fig_leaf_pixels)},
    {"Loincloth",   EquipSlot::PANTS,     0,   9,   18,  9,   1,    ICON(image_item_loincloth_pixels)},
    {"Steel Legs",  EquipSlot::PANTS,     0,   27,  54,  0,   2,    ICON(image_item_steel_legs_pixels)},
    {"Rune Legs",   EquipSlot::PANTS,     0,   81,  162, 0,   3,    ICON(image_item_rune_legs_pixels)},

    {"Worn Boots",  EquipSlot::BOOTS,     0,   2,   4,   0,   0,    ICON(image_item_worn_boots_pixels)},
    {"Iron Boots",  EquipSlot::BOOTS,     0,   6,   12,  0,   1,    ICON(image_item_iron_boots_pixels)},
    {"Steel Boots", EquipSlot::BOOTS,     0,   18,  36,  0,   2,    ICON(image_item_steel_boots_pixels)},
    {"Rune Boots",  EquipSlot::BOOTS,     0,   54,  108, 0,   3,    ICON(image_item_rune_boots_pixels)},

    {"Rag Gloves",  EquipSlot::GLOVES,    1,   1,   0,   0,   0,    ICON(image_item_rag_gloves_pixels),   {P(CLICK, 5)}},
    {"Iron Gloves", EquipSlot::GLOVES,    3,   3,   0,   0,   1,    ICON(image_item_iron_gloves_pixels)},
    {"Steel Grips", EquipSlot::GLOVES,    9,   9,   0,   0,   2,    ICON(image_item_steel_grips_pixels)},
    {"Rune Grips",  EquipSlot::GLOVES,    27,  27,  0,   0,   3,    ICON(image_item_rune_grips_pixels)},

    {"Dull Stick",  EquipSlot::SWORD,     5,   0,   0,   0,   0,    ICON(image_item_dull_stick_pixels),   {P(CLICK, 8)}},
    {"Good Stick",  EquipSlot::SWORD,     15,  0,   0,   0,   1,    ICON(image_item_good_stick_pixels)},
    {"Great Stick", EquipSlot::SWORD,     45,  0,   0,   0,   2,    ICON(image_item_great_stick_pixels)},
    {"Rune Blade",  EquipSlot::SWORD,     135, 0,   0,   0,   3,    ICON(image_item_rune_blade_pixels)},

    {"Wood Shield", EquipSlot::SHIELD,    0,   5,   5,   0,   0,    ICON(image_item_wood_shield_pixels)},
    {"Iron Shield", EquipSlot::SHIELD,    0,   15,  15,  0,   1,    ICON(image_item_iron_shield_pixels),  {P(CLICK, 5)}},
    {"Steel Guard", EquipSlot::SHIELD,    0,   45,  45,  0,   2,    ICON(image_item_steel_guard_pixels)},
    {"Rune Aegis",  EquipSlot::SHIELD,    0,   135, 135, 0,   3,    ICON(image_item_rune_aegis_pixels)},

    {"Tin Ring",    EquipSlot::RING,      2,   0,   0,   0,   0,    ICON(image_item_tin_ring_pixels),     {P(CLICK, 10), P(DROP, 5)}},
    {"Gold Ring",   EquipSlot::RING,      6,   0,   0,   0,   1,    ICON(image_item_gold_ring_pixels),    {P(CLICK, 20), P(DROP, 10), P(DOUBLE_MINE, 3)}},
    {"Ruby Ring",   EquipSlot::RING,      18,  0,   0,   0,   2,    ICON(image_item_ruby_ring_pixels),    {P(CLICK, 50), P(DROP, 15), P(RESPAWN_CUT, 20)}},
    {"Miner Ring",  EquipSlot::RING,      54,  0,   0,   0,   3,    ICON(image_item_miner_ring_pixels),   {P(CLICK, 120), P(DROP, 25), P(DOUBLE_MINE, 8)}},

    {"Wilson",      EquipSlot::AMULET,    0,   2,   0,   0,   0,    ICON(image_item_wilson_pixels),       {P(GOLD, 10), P(KILL_GOLD, 20)}},
    {"Jade Amulet", EquipSlot::AMULET,    0,   6,   0,   0,   1,    ICON(image_item_jade_amulet_pixels),  {P(GOLD, 20), P(MINE_XP, 5)}},
    {"Gold Amulet", EquipSlot::AMULET,    0,   18,  0,   0,   2,    ICON(image_item_gold_amulet_pixels),  {P(GOLD, 50), P(DROP, 50), P(RESPAWN_CUT, 20)}},
    {"Core Sigil",  EquipSlot::AMULET,    0,   54,  0,   0,   3,    ICON(image_item_core_sigil_pixels),   {P(GOLD, 120), P(UPGRADE_DISCOUNT, 10)}},

    // Mythic: three times the Epic piece of the slot, as each tier is of the one before
    {"Ember Helm",  EquipSlot::HELMET,    0,   162, 405, 0,   4,    ICON(image_icon_ember_helm_pixels)},
    {"Ember Mail",  EquipSlot::BODY,      0,   324, 810, 243, 4,    ICON(image_icon_ember_mail_pixels)},
    {"Ember Legs",  EquipSlot::PANTS,     0,   243, 486, 0,   4,    ICON(image_icon_ember_legs_pixels)},
    {"Ember Boots", EquipSlot::BOOTS,     0,   162, 324, 0,   4,    ICON(image_icon_ember_boots_pixels)},
    {"Ember Grips", EquipSlot::GLOVES,    81,  81,  0,   0,   4,    ICON(image_icon_ember_grips_pixels)},
    {"Ember Blade", EquipSlot::SWORD,     405, 0,   0,   0,   4,    ICON(image_icon_ember_blade_pixels)},
    {"Ember Guard", EquipSlot::SHIELD,    0,   405, 405, 0,   4,    ICON(image_icon_ember_guard_pixels)},
    {"Ember Ring",  EquipSlot::RING,      162, 0,   0,   0,   4,    ICON(image_icon_ember_ring_pixels),   {P(CLICK, 250), P(DROP, 40), P(RESPAWN_CUT, 30)}},
    {"Ember Charm", EquipSlot::AMULET,    0,   162, 0,   0,   4,    ICON(image_icon_ember_charm_pixels),  {P(GOLD, 250), P(UPGRADE_DISCOUNT, 15), P(MINE_XP, 10)}},

    //  name           slot               atk  def  hp   rgn  tier  icon                                  perks  relic                          %
    {"Trader Coin", EquipSlot::RELIC,     0,   0,   0,   0,   5,    ICON(image_icon_trader_coin_pixels),  {},    RelicEffect::UPGRADE_DISCOUNT, 10},
    {"Time Candle", EquipSlot::RELIC,     0,   0,   0,   0,   5,    ICON(image_icon_time_candle_pixels),  {},    RelicEffect::COOLDOWN_CUT,     15},
    {"Elder Bone",  EquipSlot::RELIC,     0,   0,   0,   0,   5,    ICON(image_icon_elder_bone_pixels),   {},    RelicEffect::AWARD_BOOST,      20},
    {"Golem Heart", EquipSlot::RELIC,     0,   0,   0,   0,   5,    ICON(image_icon_golem_heart_pixels),  {},    RelicEffect::HP_REGEN,         4},

    // Tools, each beside the plain piece of its slot and tier it gives up stats against
#define TOOL RelicEffect::NONE, 0, true
    {"Rusty Pick",  EquipSlot::SWORD,     3,   0,   0,   0,   0,    ICON(image_icon_rusty_pick_pixels),   {P(CLICK, 15), P(DOUBLE_MINE, 4)}, TOOL},
    {"Lucky Boots", EquipSlot::BOOTS,     0,   1,   2,   0,   0,    ICON(image_icon_lucky_boots_pixels),  {P(DROP, 20)}, TOOL},
    {"Iron Pick",   EquipSlot::SWORD,     9,   0,   0,   0,   1,    ICON(image_icon_iron_pick_pixels),    {P(CLICK, 30), P(DOUBLE_MINE, 6)}, TOOL},
    {"Miner Helm",  EquipSlot::HELMET,    0,   3,   8,   0,   1,    ICON(image_icon_miner_helm_pixels),   {P(CLICK, 25), P(MINE_XP, 5)}, TOOL},
    {"Loot Gloves", EquipSlot::GLOVES,    2,   2,   0,   0,   1,    ICON(image_icon_loot_gloves_pixels),  {P(DROP, 25), P(KILL_GOLD, 25)}, TOOL},
    {"Steel Pick",  EquipSlot::SWORD,     27,  0,   0,   0,   2,    ICON(image_icon_steel_pick_pixels),   {P(DOUBLE_MINE, 8), P(MINE_XP, 8)}, TOOL},
    {"Grave Hood",  EquipSlot::HELMET,    0,   10,  25,  0,   2,    ICON(image_icon_grave_hood_pixels),   {P(DROP, 30), P(KILL_GOLD, 40)}, TOOL},
    {"Coin Ring",   EquipSlot::RING,      10,  0,   0,   0,   2,    ICON(image_icon_coin_ring_pixels),    {P(GOLD, 40), P(UPGRADE_DISCOUNT, 8)}, TOOL},
    {"Rune Pick",   EquipSlot::SWORD,     80,  0,   0,   0,   3,    ICON(image_icon_rune_pick_pixels),    {P(CLICK, 100), P(DOUBLE_MINE, 10), P(MINE_XP, 10)}, TOOL},
    {"Scout Boots", EquipSlot::BOOTS,     0,   30,  60,  0,   3,    ICON(image_icon_scout_boots_pixels),  {P(DROP, 40), P(RESPAWN_CUT, 15)}, TOOL},
    {"Gilt Shield", EquipSlot::SHIELD,    0,   80,  80,  0,   3,    ICON(image_icon_gilt_shield_pixels),  {P(GOLD, 100), P(UPGRADE_DISCOUNT, 10)}, TOOL},
    {"Void Pick",   EquipSlot::SWORD,     240, 0,   0,   0,   4,    ICON(image_icon_void_pick_pixels),    {P(CLICK, 250), P(DOUBLE_MINE, 12), P(MINE_XP, 12)}, TOOL},
    {"Star Crown",  EquipSlot::HELMET,    0,   95,  240, 0,   4,    ICON(image_icon_star_crown_pixels),   {P(DROP, 60), P(KILL_GOLD, 100)}, TOOL},
#undef TOOL
};
static constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);

#undef ICON
#undef P

// 1 for the first relic in the table, 2 for the next...; 0 for anything else
inline uint8_t relicRank(uint8_t itemId)
{
    if (itemId >= ITEM_COUNT || ITEMS[itemId].slot != EquipSlot::RELIC)
    {
        return 0;
    }
    uint8_t rank = 1;
    for (uint8_t i = 0; i < itemId; i++)
    {
        rank += ITEMS[i].slot == EquipSlot::RELIC;
    }
    return rank;
}

// Level 1 gives the base value; every level after adds ITEM_LEVEL_GROWTH_PERMILLE of it
inline uint32_t scaleItemStat(uint16_t base, uint8_t level)
{
    if (base == 0 || level == 0)
    {
        return 0;
    }
    return (uint32_t)base * (1000 + (uint32_t)(level - 1) * ITEM_LEVEL_GROWTH_PERMILLE) / 1000;
}

#endif // ITEM_CONFIG_H
