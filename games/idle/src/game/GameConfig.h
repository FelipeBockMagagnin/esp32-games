#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"

// Gold is stored in thousandths so fractional production (e.g. 0.1/s) stays exact
static const uint32_t GOLD_SCALE = 1000;
static const uint16_t GOLD_COLOR = 0xF685;

// Manual mining
static const uint32_t MINE_XP = 1;         // Mining XP gained per click
static const uint32_t XP_PER_LEVEL = 10;   // XP needed for next level = XP_PER_LEVEL * level

// The ore mined changes every LEVELS_PER_TIER levels, both its name and its base click gold
struct OreTierDef
{
    const char *name;
    uint32_t clickAmount; // Whole gold units gained per click at this tier, before click upgrades
    uint16_t palette[3];  // Light, mid and dark shades the ore sprite is recolored to (RGB565)
};

// Every tier shares one ore sprite; the palette swap is what tells them apart, so a
// new tier needs three shades rather than another 18 KB image. Copper is the sprite's
// own colors, which is why its row matches ORE_SPRITE_PALETTE in MiningScreen.cpp.
static constexpr OreTierDef ORE_TIERS[] = {
    //  name             gold/click   light   mid     dark
    {"Copper Ore",       1,           {0xFC08, 0xD163, 0x806A}},
    {"Tin Ore",          3,           {0xCEBB, 0x8453, 0x422B}},
    {"Iron Ore",         8,           {0xCC8E, 0x92A7, 0x4945}},
    {"Silver Ore",       20,          {0xF7BF, 0xADB9, 0x5B0F}},
    {"Gold Ore",         50,          {0xFEEA, 0xE4C2, 0x8221}},
    {"Platinum Ore",     125,         {0xD7DE, 0x7E18, 0x3B0E}},
    {"Diamond Ore",      300,         {0x9F9F, 0x2D5D, 0x1231}},
    {"Mythril Ore",      750,         {0xD53F, 0x8A9D, 0x40F0}},
};
static constexpr uint8_t ORE_TIER_COUNT = sizeof(ORE_TIERS) / sizeof(ORE_TIERS[0]);
static constexpr uint8_t LEVELS_PER_TIER = 5; // Mining levels spent on each ore before the next

// Buildings that generate gold automatically
struct BuildingsDef
{
    const char *name;               // Up to 12 chars to fit the row title at text size 2
    uint32_t productionPerLevel;    // Per second, in GOLD_SCALE units (100 = 0.1/s)
    uint64_t baseCost;              // Whole gold units; 64-bit because the late tiers pass 4.3e9
    uint16_t costGrowthPercent;     // Cost multiplier per level bought (115 = +15%)
    const uint16_t *icon;           // 16x16 RGB565, black transparent
};

// Names for the BUILDINGS table below, for anything that needs to point at one row.
// The order here must match the table.
enum BuildingId : uint8_t
{
    BUILDING_PICKAXE, BUILDING_MINECART, BUILDING_DRILL, BUILDING_EXCAVATOR,
    BUILDING_ORE_MILL, BUILDING_SMELTER, BUILDING_DEEP_SHAFT, BUILDING_RUNE_FORGE,
    BUILDING_ORE_BARGE, BUILDING_TRANSMUTER
};

// Each tier costs roughly 12x the previous one and produces roughly 5.5x as much, so
// every new building takes about twice as long to reach as the last. Simulated for a
// player who mines actively for the first half hour and then idles: Pickaxe in seconds,
// Excavator ~8 min, Smelter ~50 min, Rune Forge ~4 h, Transmuter ~13 h.
static constexpr BuildingsDef BUILDINGS[] = {
    //  name            gold/s        cost            growth%  icon
    {"Pickaxe",         100,          15,             115, image_building_pickaxe_pixels},
    {"Minecart",        800,          150,            115, image_building_minecart_pixels},
    {"Drill",           5000,         1800,           115, image_building_drill_pixels},
    {"Excavator",       28000,        22000,          115, image_building_excavator_pixels},
    {"Ore Mill",        150000,       280000,         115, image_building_ore_mill_pixels},
    {"Smelter",         800000,       3600000,        115, image_building_smelter_pixels},
    {"Deep Shaft",      4200000,      30000000,       115, image_building_deep_shaft_pixels},
    {"Rune Forge",      22000000,     330000000,      115, image_building_rune_forge_pixels},
    {"Ore Barge",       115000000,    3600000000ULL,  115, image_building_ore_barge_pixels},
    {"Transmuter",      600000000,    40000000000ULL, 115, image_building_transmuter_pixels},
};
// Derived from the table, so adding a row is all it takes to add a building
static constexpr uint8_t BUILDING_COUNT = sizeof(BUILDINGS) / sizeof(BUILDINGS[0]);

// What a one-time gold upgrade boosts. PRODUCTION, CLICK and CLICK_PRODUCTION feed the
// economy; ATTACK, DEFENSE and MAX_HP feed combat; UNLOCK_ZONE opens a fighting zone.
// UpgradeScreen's CATEGORY_STYLES is indexed by this enum, so a new target needs a row there.
enum class UpgradeTarget : uint8_t
{
    PRODUCTION,
    CLICK,
    ATTACK,
    DEFENSE,
    MAX_HP,
    UNLOCK_ZONE,
    CLICK_PRODUCTION // Each click also earns bonusPercent of the gold produced per second
};

static const uint8_t NO_BUILDING_REQ = 0xFF;

// One-time upgrades bought with gold. An upgrade stays hidden until its requirements
// are met, so the shop reveals itself as the player progresses.
struct GoldUpgradeDef
{
    const char *name;         // Up to 14 chars to fit the detail box title
    const char *description;  // Wrapped over two lines of the detail box
    uint64_t cost;            // Whole gold units
    uint16_t bonusPercent;    // Added to the target (100 = +100%); bonuses of the same target stack additively.
                              // For UNLOCK_ZONE this carries the zone index instead of a percentage.
    UpgradeTarget target;
    uint8_t reqBuilding;      // Index into BUILDINGS, or NO_BUILDING_REQ
    uint16_t reqBuildingLevel; // Levels of reqBuilding needed before the upgrade appears
    uint16_t reqMiningLevel;   // Mining level needed before the upgrade appears; 0 for none
};

// Production bonuses are additive, so each is kept modest: together they reach +525%,
// which is what lets the curve above hold instead of collapsing into a few minutes.
// Combat upgrades and zone maps are gated so they land around when the zone that needs
// them opens: Caves ~11 min, Crypt ~50 min, Depths ~2.5 h, Molten Core ~6.5 h.
static constexpr GoldUpgradeDef GOLD_UPGRADES[] = {
    //  name             description                                  cost            bonus%   target                            reqBuilding           lvl  mineLvl
    {"Sharp Picks",     "Gold production is increased by 25%",        100,            25,      UpgradeTarget::PRODUCTION,        BUILDING_PICKAXE,     5,   0},
    {"Oiled Carts",     "Gold production is increased by 25%",        1500,           25,      UpgradeTarget::PRODUCTION,        BUILDING_MINECART,    5,   0},
    {"Gold Sense",      "Gold production is increased by 50%",        6000,           50,      UpgradeTarget::PRODUCTION,        BUILDING_PICKAXE,     25,  0},
    {"Deep Veins",      "Gold production is increased by 25%",        20000,          25,      UpgradeTarget::PRODUCTION,        BUILDING_DRILL,       5,   0},
    {"Refinery",        "Gold production is increased by 50%",        250000,         50,      UpgradeTarget::PRODUCTION,        BUILDING_EXCAVATOR,   5,   0},
    {"Midas Touch",     "Gold production is increased by 50%",        3000000,        50,      UpgradeTarget::PRODUCTION,        BUILDING_ORE_MILL,    5,   0},
    {"Ore Conveyor",    "Gold production is increased by 50%",        40000000,       50,      UpgradeTarget::PRODUCTION,        BUILDING_SMELTER,     5,   0},
    {"Flux Smelting",   "Gold production is increased by 75%",        500000000,      75,      UpgradeTarget::PRODUCTION,        BUILDING_DEEP_SHAFT,  5,   0},
    {"Shaft Bracing",   "Gold production is increased by 75%",        3500000000ULL,  75,      UpgradeTarget::PRODUCTION,        BUILDING_RUNE_FORGE,  5,   0},
    {"Rune Etching",    "Gold production is increased by 100%",       40000000000ULL, 100,     UpgradeTarget::PRODUCTION,        BUILDING_ORE_BARGE,   5,   0},

    {"Iron Fist",       "Gold per click is increased by 100%",        25,             100,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   0},
    {"Steel Grip",      "Gold per click is increased by 100%",        400,            100,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   5},
    {"Power Swing",     "Gold per click is increased by 150%",        5000,           150,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   10},
    {"Golden Touch",    "Gold per click is increased by 200%",        60000,          200,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   16},
    {"Crit Strike",     "Gold per click is increased by 250%",        800000,         250,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   22},
    {"Ore Sense",       "Each click also earns 1% of your gold/s",    50000,          1,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_EXCAVATOR,   1,   0},
    {"Titan Grip",      "Each click also earns 2% of your gold/s",    5000000,        2,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_SMELTER,     1,   0},
    {"Earthshaker",     "Each click also earns 3% of your gold/s",    300000000,      3,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_RUNE_FORGE,  1,   0},

    {"Tempered Edge",   "Attack is increased by 50%",                 3000,           50,      UpgradeTarget::ATTACK,            BUILDING_DRILL,       5,   0},
    {"Plated Guard",    "Defense is increased by 50%",                5000,           50,      UpgradeTarget::DEFENSE,           BUILDING_DRILL,       5,   0},
    {"Miners Vigor",    "Maximum health is increased by 100%",        150000,         100,     UpgradeTarget::MAX_HP,            BUILDING_ORE_MILL,    1,   0},
    {"Honed Steel",     "Attack is increased by 100%",                12000000,       100,     UpgradeTarget::ATTACK,            BUILDING_SMELTER,     1,   0},
    {"Bulwark",         "Defense is increased by 100%",               150000000,      100,     UpgradeTarget::DEFENSE,           BUILDING_DEEP_SHAFT,  1,   0},
    {"Deep Lungs",      "Maximum health is increased by 150%",        1500000000,     150,     UpgradeTarget::MAX_HP,            BUILDING_RUNE_FORGE,  1,   0},

    {"Cave Maps",       "Opens the Caves for fighting",               40000,          1,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_EXCAVATOR,   1,   0},
    {"Crypt Maps",      "Opens the Crypt for fighting",               1500000,        2,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_SMELTER,     1,   0},
    {"Depth Maps",      "Opens the Depths for fighting",              40000000,       3,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_DEEP_SHAFT,  1,   0},
    {"Core Maps",       "Opens the Molten Core for fighting",         1000000000,     4,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_RUNE_FORGE,  1,   0},
};
static constexpr uint8_t GOLD_UPGRADE_COUNT = sizeof(GOLD_UPGRADES) / sizeof(GOLD_UPGRADES[0]);

#endif // GAME_CONFIG_H
