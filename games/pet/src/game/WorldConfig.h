#ifndef WORLD_CONFIG_H
#define WORLD_CONFIG_H

#include <Arduino.h>

// 15x16 tiles of 16px fill the screen under the 28px header, leaving a 36px status bar
static const uint8_t GRID_COLS = 15;
static const uint8_t GRID_ROWS = 16;
static const uint16_t TILE_COUNT = GRID_COLS * GRID_ROWS;
static const uint8_t TILE_SIZE = 16;

// The world advances in fixed ticks pushed from `now`, so a stalled loop loses that time
// instead of catching up in a burst
static const unsigned long TICK_MS = 500;
static const uint16_t TICKS_PER_SECOND = 1000 / TICK_MS;

enum class TileKind : uint8_t
{
    WILD, // Overgrown: bought back one tile at a time, next to open ground
    GRASS,
    SOIL,
    CROP,
    BURROW,
    TROUGH,
    POND,
    FLOWERS,
    COUNT
};

// The opening patch, in the middle of the grid
static const uint8_t START_LEFT = 5;
static const uint8_t START_TOP = 6;
static const uint8_t START_SIZE = 5;
static const uint32_t START_COINS = 20;
static const uint32_t START_FOOD = 10;

// Clearing tile n of the wild costs CLEAR_BASE + n^3 / CLEAR_DIVISOR, so the first few are
// pocket change and the far corners are the late game, once the colony is all foxes
static const uint32_t CLEAR_BASE = 4;
static const uint32_t CLEAR_DIVISOR = 40;

struct BuildDef
{
    const char *name; // 12 chars max
    const char *note; // 28 chars max
    TileKind kind;
    uint32_t price;
    uint8_t growthPercent; // Compounds: each one already built raises the next by this much
};

enum BuildId : uint8_t
{
    BUILD_SOIL,
    BUILD_BURROW,
    BUILD_TROUGH,
    BUILD_FLOWERS,
    BUILD_POND,
};

static constexpr BuildDef BUILDS[] = {
    {"Till soil", "Ground for one crop", TileKind::SOIL, 3, 5},
    {"Burrow", "Beds for two critters", TileKind::BURROW, 25, 60},
    {"Trough", "Where critters eat; +pantry", TileKind::TROUGH, 15, 30},
    {"Flowers", "Cheers critters nearby", TileKind::FLOWERS, 10, 15},
    {"Pond", "Waters the crops around it", TileKind::POND, 60, 50},
};
static constexpr uint8_t BUILD_COUNT = sizeof(BUILDS) / sizeof(BUILDS[0]);

// Progress of a growing crop runs to GROWTH_FULL, in stages of a quarter
static const uint16_t GROWTH_FULL = 65000;
static const uint8_t CROP_STAGES = 4; // Seed, sprout, leafy, ripe
// A crop grows only while watered, by hand or from a pond within POND_RANGE tiles
static const uint8_t WATER_SECONDS = 90;
static const uint8_t POND_RANGE = 2;

struct CropDef
{
    const char *name; // 9 chars max
    uint16_t color;   // Ripe fruit, RGB565 art
    uint32_t seedPrice;
    uint16_t growSeconds;
    uint32_t food;
    uint32_t unlockPrice; // 0: known from the start
};

static constexpr CropDef CROPS[] = {
    {"Clover", 0xF71C, 1, 40, 4, 0},
    {"Carrot", 0xFBE0, 5, 150, 18, 120},
    {"Berry", 0xC8D4, 20, 360, 60, 800},
    {"Pumpkin", 0xFC40, 60, 900, 200, 4000},
    {"Moonbean", 0xAEDF, 200, 1800, 600, 25000},
};
static constexpr uint8_t CROP_COUNT = sizeof(CROPS) / sizeof(CROPS[0]);
static_assert(CROP_COUNT <= 8, "unlocked crops are a byte of flags");

static const uint32_t PANTRY_BASE = 40;
static const uint32_t PANTRY_PER_TROUGH = 60;

// Hunger and energy run on 0..NEED_MAX
static const uint16_t NEED_MAX = 60000;
static const uint16_t HUNGRY_AT = 36000;   // Heads for a trough from here
static const uint16_t TIRED_AT = 9000;     // Heads for bed below this much energy
static const uint16_t AWAKE_SECONDS = 360; // Full energy to empty
static const uint16_t SLEEP_SECONDS = 90;  // Empty to full, in a burrow; twice as long outside
static const uint8_t EAT_TICKS = 4;
static const uint16_t LEAVE_SECONDS = 240; // Starving this long, a critter leaves
static const uint8_t BURROW_ROOM = 2;
static const uint8_t MAX_CRITTERS = 24;

// Mood, 0..100, scales what a critter earns and decides whether it breeds
static const uint8_t MOOD_BASE = 40;
static const uint8_t MOOD_PER_FLOWER = 8;
static const uint8_t MOOD_FLOWER_CAP = 40;
static const uint8_t FLOWER_RANGE = 2;
static const uint8_t MOOD_HOUSED = 20;
static const uint8_t MOOD_HUNGRY = 15;
static const uint8_t MOOD_STARVING = 40;

// Every BREED_SECONDS, a species with a pair whose average mood reaches BREED_MOOD may have a
// young one, if a burrow has room
static const uint16_t BREED_SECONDS = 90;
static const uint8_t BREED_MOOD = 75;
static const uint8_t BREED_CHANCE_PERCENT = 50;

enum class CritterShape : uint8_t
{
    ROUND_EARS,
    LONG_EARS,
    STRIPE,
    NO_EARS,
    POINTY_EARS,
};

struct SpeciesDef
{
    const char *name; // 9 chars max
    uint16_t body;    // RGB565 art
    uint16_t accent;  // Ears and belly
    CritterShape shape;
    uint32_t price;   // The first one; each more of the species adds a quarter
    uint8_t reqPopulation;
    uint32_t coinsPerMinute; // At full mood
    uint32_t mealFood;
    uint8_t mealMinutes; // Full to hungry again, awake
};

static constexpr SpeciesDef SPECIES[] = {
    {"Mossmouse", 0x9CD3, 0xFDB8, CritterShape::ROUND_EARS, 15, 0, 4, 2, 3},
    {"Burrowhare", 0xC4EC, 0xFFDB, CritterShape::LONG_EARS, 300, 4, 15, 6, 4},
    {"Dewbadger", 0x7BEF, 0xFFFF, CritterShape::STRIPE, 3000, 8, 60, 20, 5},
    {"Glowmole", 0x6A4F, 0xFFE8, CritterShape::NO_EARS, 25000, 12, 220, 60, 6},
    {"Lumifox", 0xFC83, 0xFFFF, CritterShape::POINTY_EARS, 150000, 16, 800, 180, 7},
};
static constexpr uint8_t SPECIES_COUNT = sizeof(SPECIES) / sizeof(SPECIES[0]);

// Names handed out in turn, 8 chars max
static constexpr const char *CRITTER_NAMES[] = {
    "Pip", "Bramble", "Tuft", "Moss", "Clove", "Fern", "Nib", "Pebble", "Thistle", "Burr", "Wren", "Sorrel",
    "Dot", "Hazel", "Juniper", "Mallow", "Rye", "Sprig", "Tansy", "Acorn", "Bean", "Cress", "Dill", "Ember",
    "Fig", "Ginger", "Hops", "Ivy", "Kale", "Lark", "Minnow", "Nutmeg",
};
static constexpr uint8_t CRITTER_NAME_COUNT = sizeof(CRITTER_NAMES) / sizeof(CRITTER_NAMES[0]);

enum class UpgradeEffect : uint8_t
{
    GROW_PERCENT,   // Crops grow this much faster
    WATER_PERCENT,  // Watering lasts this much longer
    PANTRY_PERCENT, // Each trough stores this much more
    POND_RANGE,     // Tiles
    FLOWER_MOOD,    // Mood per flower
    YIELD_PERCENT,  // Food per harvest
    BURROW_ROOM,    // Critters per burrow
};

struct UpgradeDef
{
    const char *name; // 12 chars max
    const char *note; // 28 chars max
    uint32_t price;
    uint8_t reqPopulation;
    UpgradeEffect effect;
    uint8_t value;
};

static constexpr UpgradeDef UPGRADES[] = {
    {"Watering can", "Watering lasts twice as long", 60, 0, UpgradeEffect::WATER_PERCENT, 100},
    {"Compost", "Crops grow 25% faster", 300, 3, UpgradeEffect::GROW_PERCENT, 25},
    {"Big pantry", "Troughs store 50% more", 500, 4, UpgradeEffect::PANTRY_PERCENT, 50},
    {"Wildflowers", "Flowers cheer +4 mood", 1000, 6, UpgradeEffect::FLOWER_MOOD, 4},
    {"Wide ponds", "Ponds reach a tile further", 1500, 6, UpgradeEffect::POND_RANGE, 1},
    {"Rich soil", "Harvests give 25% more", 2500, 8, UpgradeEffect::YIELD_PERCENT, 25},
    {"Hot compost", "Crops grow 25% faster", 6000, 10, UpgradeEffect::GROW_PERCENT, 25},
    {"Cozy burrows", "Burrows sleep one more", 8000, 12, UpgradeEffect::BURROW_ROOM, 1},
};
static constexpr uint8_t UPGRADE_COUNT = sizeof(UPGRADES) / sizeof(UPGRADES[0]);
static_assert(UPGRADE_COUNT <= 16, "bought upgrades are 16 bits of flags");

// Terrain art, RGB565
namespace TerrainArt
{
    static const uint16_t WILD = 0x1A42;
    static const uint16_t WILD_LEAF = 0x2B44;
    static const uint16_t WILD_THORN = 0x4A22;
    static const uint16_t GRASS = 0x3D06;
    static const uint16_t GRASS_DARK = 0x2C04;
    static const uint16_t GRASS_LIGHT = 0x5E48;
    static const uint16_t SOIL = 0x7A85;
    static const uint16_t SOIL_DARK = 0x51A3;
    static const uint16_t SOIL_WET = 0x4163;
    static const uint16_t SPROUT = 0x5F2A;
    static const uint16_t LEAF = 0x2D45;
    static const uint16_t MOUND = 0x8B27;
    static const uint16_t MOUND_DARK = 0x6204;
    static const uint16_t HOLE = 0x2081;
    static const uint16_t WOOD = 0x92E5;
    static const uint16_t WOOD_DARK = 0x6203;
    static const uint16_t FEED = 0xDDA7;
    static const uint16_t WATER = 0x2B37;
    static const uint16_t WATER_LIGHT = 0x7DBC;
    static constexpr uint16_t PETALS[] = {0xFB56, 0xFFE0, 0xC81F, 0xFFFF};
    static const uint16_t EYE = 0x0841;
    static const uint16_t SNORE = 0xFFFF;
}

#endif // WORLD_CONFIG_H
