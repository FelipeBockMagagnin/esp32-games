#ifndef NAN_CONFIG_H
#define NAN_CONFIG_H

#include <stdint.h>

// Every enemy and boss is drawn from one of these; ui/Art.cpp maps them to the sprites
enum class Art : uint8_t
{
    DULL_BRANCH,
    COOL_BRANCH,
    GREAT_BRANCH,
    SLIME,
    FROG,
    SAD_FROG,
    FROG_WIZARD,
    SMALL_RAT,
    BIG_BAD_RAT,
    GAMER_MOUSE,
    SPIDER,
    GNOME,
    BARD_GNOME,
    GOBLIN,
    ZOMBIE,
    GNOME_BAND,
    COUNT
};

enum class Backdrop : uint8_t
{
    TUTORIAL,
    SEWERS,
    FOREST,
    COUNT
};

// ---- Energy -------------------------------------------------------------------------------

static const double ENERGY_CAP_BASE = 100;
static const double ENERGY_REGEN_BASE = 4; // Per second
static const double ENERGY_POWER_BASE = 1;
// The step one press of Confirm allocates, as a share of the cap
static const double ALLOC_STEP_SHARE = 0.05;

// ---- Training -----------------------------------------------------------------------------

enum class SkillKind : uint8_t
{
    ATTACK,
    DEFENSE
};

struct SkillDef
{
    const char *name;     // 12 chars max
    SkillKind kind;
    double statPerLevel;  // The stat it adds is statPerLevel * level^SKILL_EXPONENT
    double levelCost;     // Energy-seconds per level, before the energy's power
    int8_t parent;        // Skill whose level opens this one, -1 for none
    uint32_t parentLevel;
};

// The web game's Attack/Defence formula
static const double SKILL_EXPONENT = 1.3;
static const double BASE_ATTACK = 100;
static const double BASE_DEFENSE = 100;
// Max HP and HP regen come from the two stats, as in the web game
static const double HP_PER_ATTACK = 10;
static const double DEFENSE_PER_REGEN = 20;

static constexpr SkillDef SKILLS[] = {
    {"Ataque", SkillKind::ATTACK, 150, 50, -1, 0},
    {"Defesa", SkillKind::DEFENSE, 150, 50, -1, 0},
    {"Ataque Idle", SkillKind::ATTACK, 1000, 300, 0, 300},
    {"Escudo", SkillKind::DEFENSE, 1000, 300, 1, 300},
};
static constexpr uint8_t SKILL_COUNT = sizeof(SKILLS) / sizeof(SKILLS[0]);

// ---- Bosses -------------------------------------------------------------------------------

struct BossDef
{
    const char *name; // 14 chars max
    Art art;
    Backdrop backdrop;
};

// The ladder repeats past its end with "+N" after the name, its stats still growing
static constexpr BossDef BOSSES[] = {
    {"Galho Chato", Art::DULL_BRANCH, Backdrop::TUTORIAL},
    {"Slime", Art::SLIME, Backdrop::TUTORIAL},
    {"Galho Legal", Art::COOL_BRANCH, Backdrop::TUTORIAL},
    {"Sapo", Art::FROG, Backdrop::TUTORIAL},
    {"Galho Top", Art::GREAT_BRANCH, Backdrop::TUTORIAL},
    {"Sapo Mago", Art::FROG_WIZARD, Backdrop::TUTORIAL},
    {"Rato Pequeno", Art::SMALL_RAT, Backdrop::SEWERS},
    {"Sapo Triste", Art::SAD_FROG, Backdrop::SEWERS},
    {"Ratao Malvado", Art::BIG_BAD_RAT, Backdrop::SEWERS},
    {"Mouse Gamer", Art::GAMER_MOUSE, Backdrop::SEWERS},
    {"Aranha", Art::SPIDER, Backdrop::FOREST},
    {"Gnomo", Art::GNOME, Backdrop::FOREST},
    {"Goblin", Art::GOBLIN, Backdrop::FOREST},
    {"Gnomo Bardo", Art::BARD_GNOME, Backdrop::FOREST},
    {"Zumbi", Art::ZOMBIE, Backdrop::FOREST},
    {"Banda de Gnomo", Art::GNOME_BAND, Backdrop::FOREST},
};
static constexpr uint8_t BOSS_TABLE_COUNT = sizeof(BOSSES) / sizeof(BOSSES[0]);

// Boss i: attack BOSS_ATTACK_0 * BOSS_GROWTH^i, defense and HP in fixed ratios to it.
// The web game's ladder: 5e4 attack, 4e4 defense, 5e5 HP, about x5 per boss
static const double BOSS_ATTACK_0 = 5e4;
static const double BOSS_GROWTH = 4;
static const double BOSS_DEFENSE_RATIO = 0.8;
static const double BOSS_HP_RATIO = 10;
static const double BOSS_REGEN_SHARE = 0.01; // Of its max HP per second
static const double BOSS_XP_0 = 2;
static const double BOSS_XP_GROWTH = 1.5;

// ---- Spend XP -----------------------------------------------------------------------------

enum class XpUpgrade : uint8_t
{
    CAP,
    REGEN,
    POWER,
    LUCK,
    COUNT
};

struct XpUpgradeDef
{
    const char *name; // 12 chars max
    double gain;      // Per purchase
    double baseCost;
    double costGrowth;
};

static constexpr XpUpgradeDef XP_UPGRADES[] = {
    {"Energia max", 50, 2, 1.12},
    {"Regeneracao", 2, 2, 1.12},
    {"Poder", 0.25, 4, 1.18},
    {"Sorte", 0.05, 6, 1.25},
};
static_assert(sizeof(XP_UPGRADES) / sizeof(XP_UPGRADES[0]) == (uint8_t)XpUpgrade::COUNT, "one row per XpUpgrade");

// ---- Items --------------------------------------------------------------------------------

enum class Slot : uint8_t
{
    HEAD,
    BODY,
    LEGS,
    FEET,
    WEAPON,
    ACCESSORY,
    COUNT
};

// Two accessories are worn at once, so the equipment has one more place than there are slots
static const uint8_t EQUIP_COUNT = (uint8_t)Slot::COUNT + 1;

enum class ItemArt : uint8_t
{
    LEAF_HAT,
    LEATHER_CAP,
    FIG_LEAF,
    LOINCLOTH,
    DULL_STICK,
    GOOD_STICK,
    GREAT_STICK,
    WILSON,
    BOOTS,
    BOOTS_2,
    ARMOR,
    ARMOR_2,
    RING,
    NECKLACE,
    GLOVES,
    SWORD,
    COUNT
};

struct ItemDef
{
    const char *name; // 14 chars max
    Slot slot;
    ItemArt art;
    double attack;  // Percent at level 1
    double defense; // Percent at level 1
};

static constexpr ItemDef ITEMS[] = {
    {"Galho Seco", Slot::WEAPON, ItemArt::DULL_STICK, 8, 0},
    {"Chapeu Folha", Slot::HEAD, ItemArt::LEAF_HAT, 0, 6},
    {"Folha", Slot::LEGS, ItemArt::FIG_LEAF, 0, 6},
    {"Luva Velha", Slot::ACCESSORY, ItemArt::GLOVES, 4, 2},
    {"Galho Bom", Slot::WEAPON, ItemArt::GOOD_STICK, 20, 0},
    {"Capacete", Slot::HEAD, ItemArt::LEATHER_CAP, 0, 15},
    {"Tanguinha", Slot::LEGS, ItemArt::LOINCLOTH, 0, 15},
    {"Bota de Couro", Slot::FEET, ItemArt::BOOTS, 5, 10},
    {"Wilson", Slot::ACCESSORY, ItemArt::WILSON, 12, 12},
    {"Galho Otimo", Slot::WEAPON, ItemArt::GREAT_STICK, 50, 0},
    {"Armadura", Slot::BODY, ItemArt::ARMOR, 0, 40},
    {"Bota Veloz", Slot::FEET, ItemArt::BOOTS_2, 15, 25},
    {"Anel", Slot::ACCESSORY, ItemArt::RING, 30, 10},
    {"Colar", Slot::ACCESSORY, ItemArt::NECKLACE, 10, 30},
    {"Espada", Slot::WEAPON, ItemArt::SWORD, 110, 0},
    {"Cota de Malha", Slot::BODY, ItemArt::ARMOR_2, 0, 90},
};
static constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);
// A duplicate drop raises the owned item a level, up to this
static const uint8_t ITEM_MAX_LEVEL = 100;
static const double ITEM_GROWTH_PER_LEVEL = 0.1;

// ---- Adventure ----------------------------------------------------------------------------

struct EnemyDef
{
    const char *name; // 14 chars max
    Art art;
    // Multiples of the zone's reference boss
    double attack;
    double defense;
    double hp;
};

struct DropDef
{
    int8_t item; // Index into ITEMS
    uint16_t chance; // Basis points
};

static const uint8_t ZONE_ENEMIES = 4;
static const uint8_t ZONE_DROPS = 7;

struct ZoneDef
{
    const char *name; // 12 chars max
    Backdrop backdrop;
    uint16_t reqBosses; // Bosses beaten this run to enter
    uint16_t refBoss;   // Enemy stats are multiples of this boss's
    EnemyDef enemies[ZONE_ENEMIES];
    EnemyDef boss;      // Comes every ZONE_BOSS_EVERY kills
    DropDef drops[ZONE_DROPS];
    double gold;        // Per regular kill, the boss pays BOSS_GOLD_FACTOR times it
    double xp;          // Per kill that rolls XP_DROP_CHANCE
};

static const uint8_t ZONE_BOSS_EVERY = 10;
static const double BOSS_GOLD_FACTOR = 8;
static const uint16_t XP_DROP_CHANCE = 300;
static const uint16_t BOSS_DROP_BONUS = 4; // The zone boss rolls its drops at this many times the chance
static const double ADV_PLAYER_SWING_S = 1.0;
static const double ADV_STRIKE_COOLDOWN_S = 3.0;
static const double ADV_STRIKE_MULTIPLIER = 3.0;
static const double ADV_ENEMY_SWING_S = 1.5;
static const double ADV_RESPAWN_S = 1.0;

static constexpr ZoneDef ZONES[] = {
    {"Tutorial", Backdrop::TUTORIAL, 4, 3,
     {{"Galho Chato", Art::DULL_BRANCH, 0.5, 0.5, 0.05},
      {"Galho Legal", Art::COOL_BRANCH, 0.6, 0.6, 0.06},
      {"Slime", Art::SLIME, 0.55, 0.7, 0.07},
      {"Galho Top", Art::GREAT_BRANCH, 0.7, 0.7, 0.08}},
     {"Sapo Mago", Art::FROG_WIZARD, 1.0, 1.0, 0.5},
     {{0, 400}, {1, 400}, {2, 400}, {3, 200}, {-1, 0}, {-1, 0}, {-1, 0}},
     5, 1},
    {"Esgoto", Backdrop::SEWERS, 9, 7,
     {{"Rato Pequeno", Art::SMALL_RAT, 0.5, 0.5, 0.05},
      {"Sapo", Art::FROG, 0.6, 0.6, 0.06},
      {"Sapo Triste", Art::SAD_FROG, 0.55, 0.7, 0.07},
      {"Ratao Malvado", Art::BIG_BAD_RAT, 0.7, 0.7, 0.08}},
     {"Mouse Gamer", Art::GAMER_MOUSE, 1.0, 1.0, 0.5},
     {{4, 300}, {5, 300}, {6, 300}, {7, 300}, {8, 100}, {-1, 0}, {-1, 0}},
     400, 20},
    {"Floresta", Backdrop::FOREST, 13, 11,
     {{"Aranha", Art::SPIDER, 0.5, 0.5, 0.05},
      {"Gnomo", Art::GNOME, 0.6, 0.6, 0.06},
      {"Goblin", Art::GOBLIN, 0.55, 0.7, 0.07},
      {"Zumbi", Art::ZOMBIE, 0.7, 0.7, 0.08}},
     {"Banda de Gnomo", Art::GNOME_BAND, 1.0, 1.0, 0.5},
     {{9, 250}, {10, 250}, {11, 250}, {12, 200}, {13, 200}, {14, 60}, {15, 60}},
     30000, 400},
};
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

// ---- Gold: miners and augments ------------------------------------------------------------

struct MinerDef
{
    const char *name; // 12 chars max
    double cost;
    double goldPerSecond;
};

static const double MINER_COST_GROWTH = 1.15;

static constexpr MinerDef MINERS[] = {
    {"Calculadora", 15, 0.5},
    {"Raspberry Pi", 200, 4},
    {"PC Gamer", 3000, 35},
    {"Rig de GPU", 50000, 300},
    {"Data Center", 1e6, 3000},
};
static constexpr uint8_t MINER_COUNT = sizeof(MINERS) / sizeof(MINERS[0]);

enum class AugmentEffect : uint8_t
{
    ATTACK,
    DEFENSE,
    POWER // Multiplies the energy's power
};

struct AugmentDef
{
    const char *name; // 12 chars max
    AugmentEffect effect;
    double percentPerLevel;
    double levelCost; // Energy-seconds per level, before the energy's power
    double goldCost;  // For level 1; each level costs AUGMENT_GOLD_GROWTH times the last
};

static const double AUGMENT_GOLD_GROWTH = 1.35;

static constexpr AugmentDef AUGMENTS[] = {
    {"Cafe", AugmentEffect::ATTACK, 10, 400, 50},
    {"Teclado RGB", AugmentEffect::DEFENSE, 10, 400, 50},
    {"Overclock", AugmentEffect::POWER, 5, 1500, 500},
};
static constexpr uint8_t AUGMENT_COUNT = sizeof(AUGMENTS) / sizeof(AUGMENTS[0]);

// ---- Rebirth ------------------------------------------------------------------------------

// A rebirth pays (bosses beaten - REBIRTH_MIN_BOSSES + 1)^REBIRTH_EXPONENT NaN points; each
// point adds REBIRTH_PERCENT to attack, defense and the energy's power
static const uint16_t REBIRTH_MIN_BOSSES = 12;
static const double REBIRTH_EXPONENT = 2;
static const double REBIRTH_PERCENT = 10;

// ---- Unlocks ------------------------------------------------------------------------------

enum Feature : uint8_t
{
    FEAT_BOSS,
    FEAT_XP,
    FEAT_ADVENTURE, // Opens the inventory with it
    FEAT_MINERS,
    FEAT_AUGMENTS,
    FEAT_REBIRTH,
    FEATURE_COUNT
};

static const double BOSS_UNLOCK_ATTACK = 1e4;
static const uint16_t XP_UNLOCK_BOSSES = 2;
static const uint16_t AUGMENT_UNLOCK_BOSSES = 7;
// Adventure opens with ZONES[0].reqBosses, miners with the first gold, rebirth at
// REBIRTH_MIN_BOSSES

#endif // NAN_CONFIG_H
