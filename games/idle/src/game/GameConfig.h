#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>

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
};

static constexpr OreTierDef ORE_TIERS[] = {
    //  name             gold/click
    {"Copper Ore",       1},
    {"Tin Ore",          2},
    {"Iron Ore",         4},
    {"Silver Ore",       8},
    {"Gold Ore",         16},
    {"Platinum Ore",     32},
    {"Diamond Ore",      64},
    {"Mythril Ore",      128},
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
};

// Costs and rates follow Cookie Clicker's curve: each tier costs roughly 10x the
// previous one and produces roughly 5.5x as much
static constexpr BuildingsDef BUILDINGS[] = {
    //  name            gold/s        cost            growth%
    {"Pickaxe",         100,          10,             115},
    {"Minecart",        1000,         100,            115},
    {"Drill",           8000,         1100,           115},
    {"Excavator",       47000,        12000,          115},
    {"Ore Mill",        260000,       130000,         115},
    {"Smelter",         1400000,      1400000,        115},
    {"Deep Shaft",      7800000,      20000000,       115},
    {"Rune Forge",      44000000,     330000000,      115},
    {"Ore Barge",       260000000,    5100000000ULL,  115},
    {"Transmuter",      1600000000,   75000000000ULL, 115},
};
// Derived from the table, so adding a row is all it takes to add a building
static constexpr uint8_t BUILDING_COUNT = sizeof(BUILDINGS) / sizeof(BUILDINGS[0]);

// What a one-time gold upgrade boosts. PRODUCTION and CLICK feed the economy;
// ATTACK, DEFENSE and MAX_HP feed combat; UNLOCK_ZONE opens a fighting zone.
enum class UpgradeTarget : uint8_t
{
    PRODUCTION,
    CLICK,
    ATTACK,
    DEFENSE,
    MAX_HP,
    UNLOCK_ZONE
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

static constexpr GoldUpgradeDef GOLD_UPGRADES[] = {
    //  name             description                                  cost            bonus%   target                       reqBuilding        lvl  mineLvl
    {"Sharp Picks",     "Gold production is increased by 30%",        10,             30,      UpgradeTarget::PRODUCTION,   0,                 1,   0},
    {"Oiled Carts",     "Gold production is increased by 50%",        50,             50,      UpgradeTarget::PRODUCTION,   1,                 1,   0},
    {"Gold Sense",      "Miners find twice as much gold, +100%",      250,            100,     UpgradeTarget::PRODUCTION,   0,                 10,  0},
    {"Deep Veins",      "Gold production is increased by 30%",        1000,           30,      UpgradeTarget::PRODUCTION,   2,                 1,   0},
    {"Refinery",        "Gold production is increased by 150%",       5000,           150,     UpgradeTarget::PRODUCTION,   1,                 10,  0},
    {"Midas Touch",     "Gold production is increased by 200%",       25000,          200,     UpgradeTarget::PRODUCTION,   3,                 1,   0},
    {"Ore Conveyor",    "Gold production is increased by 100%",       150000,         100,     UpgradeTarget::PRODUCTION,   4,                 1,   0},
    {"Flux Smelting",   "Gold production is increased by 150%",       2000000,        150,     UpgradeTarget::PRODUCTION,   5,                 1,   0},
    {"Shaft Bracing",   "Gold production is increased by 200%",       30000000,       200,     UpgradeTarget::PRODUCTION,   6,                 1,   0},
    {"Rune Etching",    "Gold production is increased by 250%",       500000000,      250,     UpgradeTarget::PRODUCTION,   7,                 1,   0},

    {"Iron Fist",       "Gold per click is increased by 50%",         15,             50,      UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   0},
    {"Steel Grip",      "Gold per click is increased by 50%",         75,             50,      UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   3},
    {"Power Swing",     "Gold per click is increased by 100%",        375,            100,     UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   6},
    {"Golden Touch",    "Gold per click is increased by 100%",        1500,           100,     UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   10},
    {"Crit Strike",     "Gold per click is increased by 150%",        7500,           150,     UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   15},
    {"Ore Sense",       "Gold per click is increased by 200%",        30000,          200,     UpgradeTarget::CLICK,        NO_BUILDING_REQ,   0,   20},
    {"Titan Grip",      "Gold per click is increased by 250%",        200000,         250,     UpgradeTarget::CLICK,        4,                 5,   0},
    {"Earthshaker",     "Gold per click is increased by 300%",        3000000,        300,     UpgradeTarget::CLICK,        5,                 5,   0},

    {"Tempered Edge",   "Attack is increased by 50%",                 5000,           50,      UpgradeTarget::ATTACK,       2,                 5,   0},
    {"Plated Guard",    "Defense is increased by 50%",                8000,           50,      UpgradeTarget::DEFENSE,      2,                 5,   0},
    {"Miners Vigor",    "Maximum health is increased by 100%",        12000,          100,     UpgradeTarget::MAX_HP,       3,                 5,   0},
    {"Honed Steel",     "Attack is increased by 100%",                500000,         100,     UpgradeTarget::ATTACK,       4,                 10,  0},
    {"Bulwark",         "Defense is increased by 100%",               800000,         100,     UpgradeTarget::DEFENSE,      5,                 10,  0},
    {"Deep Lungs",      "Maximum health is increased by 150%",        5000000,        150,     UpgradeTarget::MAX_HP,       6,                 5,   0},

    {"Cave Maps",       "Opens the Caves for fighting",               2000,           1,       UpgradeTarget::UNLOCK_ZONE,  2,                 1,   0},
    {"Crypt Maps",      "Opens the Crypt for fighting",               100000,         2,       UpgradeTarget::UNLOCK_ZONE,  4,                 1,   0},
    {"Depth Maps",      "Opens the Depths for fighting",              3000000,        3,       UpgradeTarget::UNLOCK_ZONE,  5,                 5,   0},
    {"Core Maps",       "Opens the Molten Core for fighting",         100000000,      4,       UpgradeTarget::UNLOCK_ZONE,  7,                 1,   0},
};
static constexpr uint8_t GOLD_UPGRADE_COUNT = sizeof(GOLD_UPGRADES) / sizeof(GOLD_UPGRADES[0]);

#endif // GAME_CONFIG_H
