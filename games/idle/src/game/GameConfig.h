#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"

// Production is counted in thousandths so fractional rates (e.g. 0.1/s) stay exact
static const uint32_t GOLD_SCALE = 1000;

// Manual mining
static const uint32_t MINE_XP = 1;         // Mining XP gained per click
static const uint32_t XP_PER_LEVEL = 10;   // XP needed for next level = XP_PER_LEVEL * level

// The ore mined changes with the mining level, both its name and its base click gold
struct OreTierDef
{
    const char *name;     // Up to 20 chars, the size-2 line's width
    uint16_t minLevel;    // Mining level the ore is first mined at; ascending
    uint32_t clickAmount; // Whole gold units gained per click at this tier, before click upgrades
    uint16_t palette[3];  // Light, mid and dark shades the ore sprite is recolored to (RGB565)
};

// Every tier shares one ore sprite; the palette swap is what tells them apart, so a
// new tier needs three shades rather than another 18 KB image. Copper is the sprite's
// own colors, which is why its row matches ORE_SPRITE_PALETTE in MiningScreen.cpp.
// The first eight come every 5 levels, within the first active session; from Adamantite
// the gaps widen so the last ore lands around day 7 of the simulator's scripted player.
static constexpr OreTierDef ORE_TIERS[] = {
    //  name             level  gold/click   light   mid     dark
    {"Copper Ore",       1,     1,           {0xFC08, 0xD163, 0x806A}},
    {"Tin Ore",          6,     3,           {0xCEBB, 0x8453, 0x422B}},
    {"Iron Ore",         11,    8,           {0xCC8E, 0x92A7, 0x4945}},
    {"Silver Ore",       16,    20,          {0xF7BF, 0xADB9, 0x5B0F}},
    {"Gold Ore",         21,    50,          {0xFEEA, 0xE4C2, 0x8221}},
    {"Platinum Ore",     26,    125,         {0xD7DE, 0x7E18, 0x3B0E}},
    {"Diamond Ore",      31,    300,         {0x9F9F, 0x2D5D, 0x1231}},
    {"Mythril Ore",      36,    750,         {0xD53F, 0x8A9D, 0x40F0}},
    {"Adamantite Ore",   45,    1500,        {0x7F51, 0x2D29, 0x0A84}},
    {"Orichalcum Ore",   55,    3000,        {0xFD6B, 0xDB43, 0x7961}},
    {"Runite Ore",       66,    6000,        {0x6F1A, 0x1CB2, 0x0A29}},
    {"Obsidian Ore",     78,    12000,       {0x93F5, 0x418A, 0x1884}},
    {"Starmetal Ore",    91,    24000,       {0xEF9F, 0x953A, 0x428F}},
    {"Voidstone Ore",    105,   48000,       {0xC37F, 0x7118, 0x284A}},
    {"Celestium Ore",    120,   96000,       {0xFF94, 0x767F, 0x2AD5}},
    {"Aetherium Ore",    136,   192000,      {0xFCDC, 0xD9F4, 0x68AA}},
};
static constexpr uint8_t ORE_TIER_COUNT = sizeof(ORE_TIERS) / sizeof(ORE_TIERS[0]);

// Gold veins, the golden cookie of this game: every few minutes a vein glints beside the
// ore on the Mining screen for a short while, and pressing Up there catches it for a random
// bonus. They reward checking in, and the VEIN_* upgrades make them come more often and
// hit harder. The intervals and durations below are the base, before those upgrades.
enum class VeinEffect : uint8_t
{
    NONE,
    LUCKY,       // Instant gold: VEIN_LUCKY_BANK_PERCENT of the balance, capped at
                 // VEIN_LUCKY_SECONDS of production, plus a few clicks' worth
    FRENZY,      // Production x VEIN_FRENZY_MULTIPLIER for VEIN_FRENZY_MS
    CLICK_FRENZY // Clicks x VEIN_CLICK_MULTIPLIER for VEIN_CLICK_MS
};

static const unsigned long VEIN_MIN_INTERVAL_MS = 4UL * 60 * 1000;
static const unsigned long VEIN_MAX_INTERVAL_MS = 9UL * 60 * 1000;
static const unsigned long VEIN_VISIBLE_MS = 15000;
static const uint8_t VEIN_LUCKY_BANK_PERCENT = 15;
static const uint32_t VEIN_LUCKY_SECONDS = 900;
static const uint8_t VEIN_LUCKY_CLICKS = 13;
static const uint8_t VEIN_FRENZY_MULTIPLIER = 7;
static const unsigned long VEIN_FRENZY_MS = 77000;
static const uint8_t VEIN_CLICK_MULTIPLIER = 10;
static const unsigned long VEIN_CLICK_MS = 13000;
// Out of 100: what a caught vein turns out to be
static const uint8_t VEIN_LUCKY_WEIGHT = 50;
static const uint8_t VEIN_FRENZY_WEIGHT = 40; // The rest is CLICK_FRENZY

// Buildings that generate gold automatically
struct BuildingsDef
{
    const char *name;               // Up to 12 chars to fit the row title at text size 2
    uint64_t productionPerLevel;    // Per second, in GOLD_SCALE units (100 = 0.1/s)
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

// Tuned with tools/sim for a run of about three days (see CLAUDE.md for the targets):
// each tier costs 16.5x the previous one and produces 7x as much, so a tier pays for
// itself a bit over twice as slowly as the one before it. First of each, for a player
// checking in hourly and catching the gold veins that show while they are there:
// Excavator ~15 min, Ore Mill 1 h, Smelter 4 h, Deep Shaft 8 h, Rune Forge 15 h, Ore
// Barge 31 h, Transmuter ~57 h. A player who checks in for less sees fewer veins and
// takes closer to four days. A boost costs up to 1e6x its building's base, which kept
// the last base cost near a trillion while a balance capped at 1.8e16 (it now holds 1.8e19).
static constexpr BuildingsDef BUILDINGS[] = {
    //  name         gold/s          cost              growth%  icon
    {"Pickaxe",      100,            15,               115,     image_building_pickaxe_pixels},
    {"Minecart",     700,            250,              115,     image_building_minecart_pixels},
    {"Drill",        4900,           4100,             115,     image_building_drill_pixels},
    {"Excavator",    34000,          67000,            115,     image_building_excavator_pixels},
    {"Ore Mill",     240000,         1100000,          115,     image_building_ore_mill_pixels},
    {"Smelter",      1700000,        18000000,         115,     image_building_smelter_pixels},
    {"Deep Shaft",   12000000,       300000000,        115,     image_building_deep_shaft_pixels},
    {"Rune Forge",   82000000,       5000000000ULL,    115,     image_building_rune_forge_pixels},
    {"Ore Barge",    580000000,      82000000000ULL,   115,     image_building_ore_barge_pixels},
    {"Transmuter",   4000000000ULL,  1400000000000ULL, 115,     image_building_transmuter_pixels},
};
// Derived from the table, so adding a row is all it takes to add a building
static constexpr uint8_t BUILDING_COUNT = sizeof(BUILDINGS) / sizeof(BUILDINGS[0]);

// What a one-time gold upgrade boosts. PRODUCTION, CLICK, CLICK_PRODUCTION and
// BUILDING_BOOST feed the economy; ATTACK, DEFENSE and MAX_HP feed combat; UNLOCK_ZONE
// opens a fighting zone.
// UpgradeScreen's CATEGORY_STYLES is indexed by this enum, so a new target needs a row there.
enum class UpgradeTarget : uint8_t
{
    PRODUCTION,
    CLICK,
    ATTACK,
    DEFENSE,
    MAX_HP,
    UNLOCK_ZONE,
    CLICK_PRODUCTION, // Each click also earns bonusPercent of the gold produced per second
    BUILDING_BOOST,   // Doubles the output of BUILDINGS[reqBuilding]; boosts multiply together
    VEIN_FREQUENCY,   // Gold veins come bonusPercent more often
    VEIN_POWER        // Vein bonuses last longer and Lucky pays more, by bonusPercent
};

static const uint8_t NO_BUILDING_REQ = 0xFF;

// Cookie-Clicker-style building upgrades: five per building, each doubling that building's
// output, offered once the player owns BOOST_LEVELS of it and priced at BOOST_COST_FACTORS
// times its base cost. They multiply, so a building's late upgrades are what keep it
// relevant after the next tier arrives.
static constexpr uint16_t BOOST_LEVELS[] = {1, 10, 25, 50, 100};
static constexpr uint64_t BOOST_COST_FACTORS[] = {10, 100, 1000, 50000, 1000000};

// One-time upgrades bought with gold. An upgrade stays hidden until its requirements
// are met, so the shop reveals itself as the player progresses.
struct GoldUpgradeDef
{
    const char *name;         // Up to 13 chars, so the size-2 row title clears the price
    const char *description;  // Wrapped over two 36-char lines of the detail panel
    uint64_t cost;            // Whole gold units
    uint16_t bonusPercent;    // Added to the target (100 = +100%); bonuses of the same target stack additively.
                              // For UNLOCK_ZONE this carries the zone index instead of a percentage.
    UpgradeTarget target;
    uint8_t reqBuilding;      // Index into BUILDINGS, or NO_BUILDING_REQ
    uint16_t reqBuildingLevel; // Levels of reqBuilding needed before the upgrade appears
    uint16_t reqMiningLevel;   // Mining level needed before the upgrade appears; 0 for none
};

// Prices are multiples of the base cost of the building that gates them, so retuning the
// buildings carries the whole shop along. Production bonuses are additive and kept small
// (+25% each, +250% together): the multiplying building boosts below do the heavy lifting.
// Combat upgrades and zone maps are gated so they land around when the zone that needs
// them opens.
#define AT(building, factor) (BUILDINGS[building].baseCost * (factor))

// The five boosts of one building, named after it ("Pickaxe I" to "Pickaxe V")
#define BOOST_ROW(id, NAME, PLURAL, tier, numeral)                                             \
    {NAME " " numeral, PLURAL " produce twice as much gold",                                     \
     BUILDINGS[id].baseCost * BOOST_COST_FACTORS[tier], 100, UpgradeTarget::BUILDING_BOOST, id, \
     BOOST_LEVELS[tier], 0}
#define BUILDING_BOOSTS(id, NAME, PLURAL)                                                      \
    BOOST_ROW(id, NAME, PLURAL, 0, "I"), BOOST_ROW(id, NAME, PLURAL, 1, "II"),                 \
        BOOST_ROW(id, NAME, PLURAL, 2, "III"), BOOST_ROW(id, NAME, PLURAL, 3, "IV"),           \
        BOOST_ROW(id, NAME, PLURAL, 4, "V")

static constexpr GoldUpgradeDef GOLD_UPGRADES[] = {
    //  name             description                                  cost                          bonus%   target                            reqBuilding           lvl  mineLvl
    {"Sharp Picks",     "Gold production is increased by 25%",        AT(BUILDING_PICKAXE, 40),     25,      UpgradeTarget::PRODUCTION,        BUILDING_PICKAXE,     15,  0},
    {"Oiled Carts",     "Gold production is increased by 25%",        AT(BUILDING_MINECART, 40),    25,      UpgradeTarget::PRODUCTION,        BUILDING_MINECART,    15,  0},
    {"Deep Veins",      "Gold production is increased by 25%",        AT(BUILDING_DRILL, 40),       25,      UpgradeTarget::PRODUCTION,        BUILDING_DRILL,       15,  0},
    {"Refinery",        "Gold production is increased by 25%",        AT(BUILDING_EXCAVATOR, 40),   25,      UpgradeTarget::PRODUCTION,        BUILDING_EXCAVATOR,   15,  0},
    {"Midas Touch",     "Gold production is increased by 25%",        AT(BUILDING_ORE_MILL, 40),    25,      UpgradeTarget::PRODUCTION,        BUILDING_ORE_MILL,    15,  0},
    {"Ore Conveyor",    "Gold production is increased by 25%",        AT(BUILDING_SMELTER, 40),     25,      UpgradeTarget::PRODUCTION,        BUILDING_SMELTER,     15,  0},
    {"Flux Smelting",   "Gold production is increased by 25%",        AT(BUILDING_DEEP_SHAFT, 40),  25,      UpgradeTarget::PRODUCTION,        BUILDING_DEEP_SHAFT,  15,  0},
    {"Shaft Bracing",   "Gold production is increased by 25%",        AT(BUILDING_RUNE_FORGE, 40),  25,      UpgradeTarget::PRODUCTION,        BUILDING_RUNE_FORGE,  15,  0},
    {"Rune Etching",    "Gold production is increased by 25%",        AT(BUILDING_ORE_BARGE, 40),   25,      UpgradeTarget::PRODUCTION,        BUILDING_ORE_BARGE,   15,  0},
    {"Gold Sense",      "Gold production is increased by 25%",        AT(BUILDING_TRANSMUTER, 40),  25,      UpgradeTarget::PRODUCTION,        BUILDING_TRANSMUTER,  15,  0},

    {"Iron Fist",       "Gold per click is increased by 100%",        50,                           100,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   0},
    {"Steel Grip",      "Gold per click is increased by 100%",        600,                          100,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   5},
    {"Power Swing",     "Gold per click is increased by 150%",        8000,                         150,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   10},
    {"Golden Touch",    "Gold per click is increased by 200%",        150000,                       200,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   16},
    {"Crit Strike",     "Gold per click is increased by 250%",        3000000,                      250,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   22},
    {"Diamond Edge",    "Gold per click is increased by 300%",        100000000,                    300,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   30},
    {"Mythril Pick",    "Gold per click is increased by 400%",        5000000000ULL,                400,     UpgradeTarget::CLICK,             NO_BUILDING_REQ,      0,   40},
    {"Ore Sense",       "Each click also earns 1% of your gold/s",    AT(BUILDING_EXCAVATOR, 3),    1,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_EXCAVATOR,   1,   0},
    {"Titan Grip",      "Each click also earns 2% of your gold/s",    AT(BUILDING_SMELTER, 3),      2,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_SMELTER,     1,   0},
    {"Earthshaker",     "Each click also earns 3% of your gold/s",    AT(BUILDING_RUNE_FORGE, 3),   3,       UpgradeTarget::CLICK_PRODUCTION,  BUILDING_RUNE_FORGE,  1,   0},

    {"Tempered Edge",   "Attack is increased by 50%",                 AT(BUILDING_DRILL, 2),        50,      UpgradeTarget::ATTACK,            BUILDING_DRILL,       5,   0},
    {"Plated Guard",    "Defense is increased by 50%",                AT(BUILDING_DRILL, 3),        50,      UpgradeTarget::DEFENSE,           BUILDING_DRILL,       5,   0},
    {"Miners Vigor",    "Maximum health is increased by 100%",        AT(BUILDING_ORE_MILL, 2),     100,     UpgradeTarget::MAX_HP,            BUILDING_ORE_MILL,    1,   0},
    {"Honed Steel",     "Attack is increased by 100%",                AT(BUILDING_SMELTER, 2),      100,     UpgradeTarget::ATTACK,            BUILDING_SMELTER,     1,   0},
    {"Bulwark",         "Defense is increased by 100%",               AT(BUILDING_DEEP_SHAFT, 2),   100,     UpgradeTarget::DEFENSE,           BUILDING_DEEP_SHAFT,  1,   0},
    {"Deep Lungs",      "Maximum health is increased by 150%",        AT(BUILDING_RUNE_FORGE, 2),   150,     UpgradeTarget::MAX_HP,            BUILDING_RUNE_FORGE,  1,   0},
    {"Runic Edge",      "Attack is increased by 100%",                AT(BUILDING_ORE_BARGE, 1),    100,     UpgradeTarget::ATTACK,            BUILDING_ORE_BARGE,   1,   0},
    {"Prospecting",     "Gold veins appear 50% more often",           AT(BUILDING_DRILL, 5),        50,      UpgradeTarget::VEIN_FREQUENCY,    BUILDING_DRILL,       1,   0},
    {"Lucky Charm",     "Gold vein bonuses are 50% stronger",         AT(BUILDING_EXCAVATOR, 5),    50,      UpgradeTarget::VEIN_POWER,        BUILDING_EXCAVATOR,   1,   0},
    {"Dowsing Rod",     "Gold veins appear 50% more often",           AT(BUILDING_ORE_MILL, 5),     50,      UpgradeTarget::VEIN_FREQUENCY,    BUILDING_ORE_MILL,    1,   0},
    {"Rich Seams",      "Gold vein bonuses are 50% stronger",         AT(BUILDING_SMELTER, 5),      50,      UpgradeTarget::VEIN_POWER,        BUILDING_SMELTER,     1,   0},
    {"Serendipity",     "Gold veins appear twice as often",           AT(BUILDING_DEEP_SHAFT, 5),   100,     UpgradeTarget::VEIN_FREQUENCY,    BUILDING_DEEP_SHAFT,  1,   0},
    {"Get Lucky",       "Gold vein bonuses are twice as strong",      AT(BUILDING_RUNE_FORGE, 5),   100,     UpgradeTarget::VEIN_POWER,        BUILDING_RUNE_FORGE,  1,   0},
    {"Gold Fever",      "Gold veins appear twice as often",           AT(BUILDING_ORE_BARGE, 5),    100,     UpgradeTarget::VEIN_FREQUENCY,    BUILDING_ORE_BARGE,   1,   0},

    {"Alchemic Edge",   "Attack is increased by 100%",                AT(BUILDING_TRANSMUTER, 5),   100,     UpgradeTarget::ATTACK,            BUILDING_TRANSMUTER,  5,   0},

    {"Forest Maps",     "Opens the Forest for fighting",              AT(BUILDING_ORE_MILL, 2),     1,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_ORE_MILL,    1,   0},
    {"Crypt Maps",      "Opens the Crypt for fighting",               AT(BUILDING_DEEP_SHAFT, 2),   2,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_DEEP_SHAFT,  1,   0},
    {"Depth Maps",      "Opens the Depths for fighting",              AT(BUILDING_RUNE_FORGE, 2),   3,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_RUNE_FORGE,  1,   0},
    {"Core Maps",       "Opens the Molten Core for fighting",         AT(BUILDING_ORE_BARGE, 5),    4,       UpgradeTarget::UNLOCK_ZONE,       BUILDING_TRANSMUTER,  1,   0},

    BUILDING_BOOSTS(BUILDING_PICKAXE,    "Pickaxe",    "Pickaxes"),
    BUILDING_BOOSTS(BUILDING_MINECART,   "Minecart",   "Minecarts"),
    BUILDING_BOOSTS(BUILDING_DRILL,      "Drill",      "Drills"),
    BUILDING_BOOSTS(BUILDING_EXCAVATOR,  "Excavator",  "Excavators"),
    BUILDING_BOOSTS(BUILDING_ORE_MILL,   "Ore Mill",   "Ore Mills"),
    BUILDING_BOOSTS(BUILDING_SMELTER,    "Smelter",    "Smelters"),
    BUILDING_BOOSTS(BUILDING_DEEP_SHAFT, "Deep Shaft", "Deep Shafts"),
    BUILDING_BOOSTS(BUILDING_RUNE_FORGE, "Rune Forge", "Rune Forges"),
    BUILDING_BOOSTS(BUILDING_ORE_BARGE,  "Ore Barge",  "Ore Barges"),
    BUILDING_BOOSTS(BUILDING_TRANSMUTER, "Transmuter", "Transmuters"),
};

#undef BUILDING_BOOSTS
#undef BOOST_ROW
#undef AT
static constexpr uint8_t GOLD_UPGRADE_COUNT = sizeof(GOLD_UPGRADES) / sizeof(GOLD_UPGRADES[0]);

#endif // GAME_CONFIG_H
