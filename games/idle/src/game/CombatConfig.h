#ifndef COMBAT_CONFIG_H
#define COMBAT_CONFIG_H

#include <Arduino.h>
#include "ItemConfig.h"
#include "../assets/Assets.h"
#include "../assets/Sprites.h"

// Combat pacing. The auto-attack runs on its own clock so a fight progresses on any
// screen; the two manual actions are what the player adds by being present.
static const unsigned long AUTO_ATTACK_MS = 2000;
static const unsigned long STRIKE_COOLDOWN_MS = 1500;
static const uint8_t STRIKE_MULTIPLIER = 2; // Manual strike hits this many times as hard

// No floor: an attack that cannot beat the defense does nothing. A floor of 1 would let
// the auto-attack grind down anything, and gear would stop being the way into a zone.
inline uint32_t damageThrough(uint32_t attack, uint32_t defense)
{
    return attack > defense ? attack - defense : 0;
}

// What an enemy's hit does to the player: the defense counts half, as NGU Idle's Toughness
// does, so a new set softens the hits instead of making the player immune at once, and
// the hits fade over the set's levels. Immunity takes twice the enemy's attack.
inline uint32_t hurtThrough(uint32_t attack, uint32_t defense)
{
    return damageThrough(attack, defense / 2);
}

static const unsigned long GUARD_COOLDOWN_MS = 9000;
static const unsigned long GUARD_DURATION_MS = 3000;
static const uint8_t GUARD_DAMAGE_PERCENT = 50; // Incoming damage while guarding
static const unsigned long RESPAWN_MS = 4000; // Long enough to take in a kill and read its drop
static const unsigned long REVIVE_MS = 8000; // Health refills over this long after a death
static const unsigned long REGEN_TICK_MS = 1000;
static const uint8_t BOSS_KILL_SECONDS = 3;
// A zone's boss comes after one of each of its other enemies on average, as often as a
// random pick would bring it, so its drops keep their rates. Beaten this fast, it pays
// this many times its gold; it runs off when it kills the player, so an idle fight never
// stalls on it.
static const unsigned long BOSS_SWIFT_MS = 15000;
static const uint8_t BOSS_SWIFT_GOLD_MULTIPLIER = 2;

// What the relics' perks do (see ItemPerk). All three act on the strike only, and the
// two that add damage multiply what got through the defense, so a strike the defense blocks
// stays blocked and gear alone still decides which zones are in reach.
// A WEAK_POINT perk makes the enemy open its guard now and then; a strike in the window crits.
static const unsigned long WEAK_POINT_OPEN_MS = 1000;
static const unsigned long WEAK_POINT_MIN_GAP_MS = 4000;
static const unsigned long WEAK_POINT_MAX_GAP_MS = 7000;
// A FURY perk fills a meter by strikes landed and hits taken, the perk speeding it up; full,
// it sets the strike's cooldown to almost nothing for a short burst. It lasts across enemies
// and empties on a death.
static const uint16_t FURY_PER_STRIKE = 20;
static const uint16_t FURY_PER_HURT = 15;
static const uint16_t FURY_FULL = 100;
static const unsigned long FURY_MS = 3000;
static const unsigned long FURY_STRIKE_COOLDOWN_MS = 400;

// The equipped amulet decides what the Up button casts. No spell deals damage, so the
// strike stays the only way past a defense the auto-attack cannot beat; each one changes
// the fight in its own way instead. A row per amulet: an amulet with no row grants nothing.
enum class SpellEffect : uint8_t
{
    HEAL,  // Restores `power` percent of max HP; refused at full health
    STUN,  // Pushes the enemy's next attack back by `power` full intervals
    MIDAS, // The next kill pays `power` times its gold; refused while one is already armed
    WARD,  // No damage taken for `power` milliseconds
    SEEK   // The next kill rolls its drops at `power` times the chance; refused while armed
};

struct SpellDef
{
    uint8_t itemId;   // The amulet that grants it
    const char *name; // Up to 6 chars, shown beside the Up button
    SpellEffect effect;
    uint16_t power;   // Meaning depends on the effect, see above
    unsigned long cooldownMs;
};

static constexpr SpellDef SPELLS[] = {
    //  amulet              name     effect               power  cooldown
    {ITEM_WILSON,        "MEND",  SpellEffect::HEAL,   30,    15000},
    {ITEM_JADE_AMULET,   "STUN",  SpellEffect::STUN,   1,     12000},
    {ITEM_GOLD_AMULET,   "MIDAS", SpellEffect::MIDAS,  3,     20000},
    {ITEM_CORE_SIGIL,    "WARD",  SpellEffect::WARD,   4000,  18000},
    {ITEM_EMBER_CHARM,   "SEEK",  SpellEffect::SEEK,   3,     20000},
};
static constexpr uint8_t SPELL_COUNT = sizeof(SPELLS) / sizeof(SPELLS[0]);

// The spell an item grants when worn, or nullptr
inline const SpellDef *spellForItem(int8_t itemId)
{
    for (uint8_t i = 0; i < SPELL_COUNT; i++)
    {
        if (SPELLS[i].itemId == itemId)
        {
            return &SPELLS[i];
        }
    }
    return nullptr;
}

// Chance is in basis points, so 10000 is a guaranteed drop
struct DropDef
{
    uint8_t itemId;
    uint16_t chanceBp;
};

// An item not yet owned drops this many times as often. The first piece is what lets the
// auto-attack through a new zone; the long farm is meant for its levels, not for finding it,
// but a set still takes the best part of an hour of striking to come together.
static const uint8_t NEW_ITEM_DROP_MULTIPLIER = 2;

struct EnemyDef
{
    const char *name;
    uint32_t maxHp;
    uint32_t attack;
    uint32_t defense;
    unsigned long attackIntervalMs;
    uint64_t goldReward; // Past the Molten Core a kill pays more than 32 bits hold
    // 64x64 palette + run-length art, drawn at 2x. The Crypt, Depths and Core enemies
    // borrow a Sewers sprite until they get their own; swapping art is a pointer change.
    const SpriteDef *sprite;
    const DropDef *drops;
    uint8_t dropCount;
    bool boss; // Left out of most rows, so it zero-initializes to false
};

struct ZoneDef
{
    const char *name; // Up to 11 chars to fit a list row title at text size 2
    const EnemyDef *enemies;
    uint8_t enemyCount;
    bool requiresUnlockUpgrade; // Needs the UNLOCK_ZONE upgrade that carries this zone index
    // Seconds of base production a kill pays on top of its table gold (a boss
    // BOSS_KILL_SECONDS times as many), so fighting keeps its share of the income once the
    // buildings outgrow the table
    uint8_t killSeconds;
    const uint16_t *icon;       // 16x16 RGB565, black transparent
    const SpriteDef *background; // 64x64, drawn behind the enemy at 2x
    uint8_t reqDelveLevel;      // Deep Delver levels needed; left off the first zones' rows
};

// Drop chances are fixed; only equipped items, awards, talents, a Seek and a first find
// raise them. A repeat drop is one level, and the first levels are the big ones
// (scaleItemStat), so a zone's chances are set for its set to be found within its first
// hour and to stand around level 5-20 when the next zone's map is bought: the level the
// next zone's ladder expects its arrivals at. They fall from zone to zone, so the early
// zones rain items and the late ones are a long farm across ascensions. With N enemies
// picked at random, an item on k of them at chance c drops at k*c/N per kill. The boss of
// each zone carries its ring and amulet; a zone's tools drop a little less than its set,
// and its relic (on the boss, or the second enemy) far less: a drop is ten levels, so a
// relic still maxes in a few dozen hours of farming its zone in a later run.
static constexpr DropDef SMALL_RAT_DROPS[] = {
    {ITEM_DULL_STICK, 1400},
    {ITEM_RAG_GLOVES, 1400},
    {ITEM_RUSTY_PICK, 800},
};
static constexpr DropDef SLIME_DROPS[] = {
    {ITEM_FIG_LEAF, 1400},
    {ITEM_WORN_BOOTS, 1400},
    {ITEM_LUCKY_BOOTS, 1400},
};
static constexpr DropDef FROG_DROPS[] = {
    {ITEM_LEAF_HAT, 1400},
    {ITEM_CLOTH_VEST, 1400},
};
static constexpr DropDef SAD_FROG_DROPS[] = {
    {ITEM_DULL_STICK, 1400},
    {ITEM_FIG_LEAF, 1400},
    {ITEM_WOOD_SHIELD, 900},
};
static constexpr DropDef BIG_BAD_RAT_DROPS[] = {
    {ITEM_RAG_GLOVES, 1400},
    {ITEM_WORN_BOOTS, 1400},
    {ITEM_CLOTH_VEST, 1400},
    {ITEM_RUSTY_PICK, 800},
};
static constexpr DropDef FROG_WIZARD_DROPS[] = {
    {ITEM_LEAF_HAT, 1400},
    {ITEM_WOOD_SHIELD, 900},
    {ITEM_TIN_RING, 1000},
};
static constexpr DropDef GAMER_MOUSE_DROPS[] = {
    {ITEM_TIN_RING, 1000},
    {ITEM_WILSON, 1800},
    {ITEM_WOOD_SHIELD, 900},
    {ITEM_RAT_FANG, 50},
};

static constexpr DropDef CAVE_BAT_DROPS[] = {
    {ITEM_BRASS_HELM, 1750},
    {ITEM_BRASS_MITTS, 1750},
    {ITEM_BRASS_SWORD, 1750},
};
static constexpr DropDef MOLE_MINER_DROPS[] = {
    {ITEM_BRASS_VEST, 1750},
    {ITEM_BRASS_LEGS, 1750},
    {ITEM_BRASS_BOOTS, 1750},
    {ITEM_RUSTY_PICK, 500},
};
static constexpr DropDef ROCK_TROLL_DROPS[] = {
    {ITEM_BRASS_TARGE, 2700},
    {ITEM_BRASS_SWORD, 1750},
    {ITEM_TIN_RING, 800},
    {ITEM_WILSON, 1000},
};

static constexpr DropDef SPIDER_DROPS[] = {
    {ITEM_LEATHER_CAP, 1500},
    {ITEM_IRON_GLOVES, 1500},
    {ITEM_LOOT_GLOVES, 350},
};
static constexpr DropDef GNOME_DROPS[] = {
    {ITEM_GOOD_STICK, 1500},
    {ITEM_IRON_MAIL, 1500},
    {ITEM_IRON_PICK, 350},
};
static constexpr DropDef BARD_GNOME_DROPS[] = {
    {ITEM_LOINCLOTH, 1500},
    {ITEM_IRON_BOOTS, 1500},
    {ITEM_LEATHER_CAP, 1500},
    {ITEM_MINER_HELM, 350},
};
static constexpr DropDef GOBLIN_DROPS[] = {
    {ITEM_GOOD_STICK, 1500},
    {ITEM_IRON_GLOVES, 1500},
    {ITEM_IRON_SHIELD, 900},
};
static constexpr DropDef ZOMBIE_DROPS[] = {
    {ITEM_IRON_MAIL, 1500},
    {ITEM_LOINCLOTH, 1500},
    {ITEM_IRON_BOOTS, 1500},
};
static constexpr DropDef GNOME_BAND_DROPS[] = {
    {ITEM_IRON_SHIELD, 900},
    {ITEM_GOLD_RING, 400},
    {ITEM_JADE_AMULET, 400},
    {ITEM_HAWK_GLOVES, 30},
};

static constexpr DropDef BARROW_WIGHT_DROPS[] = {
    {ITEM_BONE_HELM, 800},
    {ITEM_BONE_GRIPS, 800},
    {ITEM_BONE_CLUB, 800},
    {ITEM_LOOT_GLOVES, 150},
};
static constexpr DropDef GRAVE_HOUND_DROPS[] = {
    {ITEM_BONE_MAIL, 800},
    {ITEM_BONE_LEGS, 800},
    {ITEM_BONE_BOOTS, 800},
};
static constexpr DropDef BARROW_KING_DROPS[] = {
    {ITEM_BONE_SHIELD, 1200},
    {ITEM_BONE_CLUB, 800},
    {ITEM_GOLD_RING, 250},
    {ITEM_JADE_AMULET, 250},
};

static constexpr DropDef BONE_DIGGER_DROPS[] = {
    {ITEM_STEEL_HELM, 300},
    {ITEM_STEEL_GRIPS, 300},
    {ITEM_GREAT_STICK, 300},
    {ITEM_STEEL_PICK, 80},
    {ITEM_GRAVE_HOOD, 80},
    {ITEM_COIN_RING, 80},
};
static constexpr DropDef CRYPT_GHOUL_DROPS[] = {
    {ITEM_STEEL_PLATE, 300},
    {ITEM_STEEL_LEGS, 300},
    {ITEM_STEEL_BOOTS, 300},
    {ITEM_STEEL_GUARD, 250},
    {ITEM_RUBY_RING, 60},
    {ITEM_GOLD_AMULET, 60},
    {ITEM_RAGE_MASK, 10},
};

static constexpr DropDef ONYX_BEETLE_DROPS[] = {
    {ITEM_ONYX_HELM, 550},
    {ITEM_ONYX_GRIPS, 550},
    {ITEM_ONYX_BLADE, 550},
    {ITEM_GRAVE_HOOD, 40},
};
static constexpr DropDef SHADE_DROPS[] = {
    {ITEM_ONYX_PLATE, 550},
    {ITEM_ONYX_LEGS, 550},
    {ITEM_ONYX_BOOTS, 550},
};
static constexpr DropDef ONYX_DRAKE_DROPS[] = {
    {ITEM_ONYX_WARD, 870},
    {ITEM_ONYX_BLADE, 550},
    {ITEM_RUBY_RING, 40},
    {ITEM_GOLD_AMULET, 40},
};

static constexpr DropDef DEEP_LURKER_DROPS[] = {
    {ITEM_RUNE_HELM, 150},
    {ITEM_RUNE_GRIPS, 150},
    {ITEM_RUNE_BLADE, 150},
    {ITEM_RUNE_PICK, 16},
};
static constexpr DropDef ABYSS_WORM_DROPS[] = {
    {ITEM_RUNE_PLATE, 150},
    {ITEM_RUNE_LEGS, 150},
    {ITEM_RUNE_BOOTS, 150},
    {ITEM_RUNE_AEGIS, 120},
    {ITEM_SCOUT_BOOTS, 16},
    {ITEM_RUNE_CLEAVE, 6},
};

static constexpr DropDef MAGMA_HOUND_DROPS[] = {
    {ITEM_MAGMA_HELM, 30},
    {ITEM_MAGMA_FISTS, 30},
    {ITEM_MAGMA_BRAND, 30},
    {ITEM_MAGMA_WALL, 25},
    {ITEM_MINER_RING, 30},
    {ITEM_GILT_SHIELD, 40},
};
static constexpr DropDef CORE_GOLEM_DROPS[] = {
    {ITEM_MAGMA_PLATE, 30},
    {ITEM_MAGMA_LEGS, 30},
    {ITEM_MAGMA_BOOTS, 30},
    {ITEM_MINER_RING, 30},
    {ITEM_CORE_SIGIL, 60},
    {ITEM_MAGMA_CLAWS, 3},
};

// The Mythic set drops in both deep zones
static constexpr DropDef RIFT_STALKER_DROPS[] = {
    {ITEM_EMBER_HELM, 20},
    {ITEM_EMBER_GRIPS, 20},
    {ITEM_EMBER_BLADE, 20},
};
static constexpr DropDef VOID_WRAITH_DROPS[] = {
    {ITEM_EMBER_MAIL, 20},
    {ITEM_EMBER_LEGS, 20},
    {ITEM_EMBER_BOOTS, 20},
    {ITEM_EMBER_GUARD, 16},
};
static constexpr DropDef VOID_MAW_DROPS[] = {
    {ITEM_EMBER_BLADE, 20},
    {ITEM_EMBER_GUARD, 20},
    {ITEM_EMBER_RING, 12},
    {ITEM_EMBER_CHARM, 12},
    {ITEM_VOID_PICK, 12},
    {ITEM_VOID_VISAGE, 4},
};
static constexpr DropDef STAR_SMITH_DROPS[] = {
    {ITEM_STAR_HELM, 14},
    {ITEM_STAR_GRIPS, 14},
    {ITEM_STAR_BLADE, 14},
    {ITEM_STAR_SHIELD, 12},
    {ITEM_VOID_PICK, 8},
};
static constexpr DropDef NOVA_GOLEM_DROPS[] = {
    {ITEM_STAR_MAIL, 14},
    {ITEM_STAR_LEGS, 14},
    {ITEM_STAR_BOOTS, 14},
    {ITEM_STAR_CROWN, 8},
};
static constexpr DropDef SUN_EATER_DROPS[] = {
    {ITEM_EMBER_RING, 10},
    {ITEM_EMBER_CHARM, 8},
    {ITEM_STAR_CROWN, 8},
    {ITEM_STAR_AEGIS, 3},
};

// Until the deeper zones get their own art
#define PLACEHOLDER_SPRITE &sprite_big_bad_rat

#define DROPS(table) table, sizeof(table) / sizeof(table[0])

// Balance rule: each zone follows the difficulty ladder (tools/sim/ladder.sh, which fights
// it with the real CombatState and the combat upgrades on sale beside its map). The earlier
// zones' gear at level 1 cannot win here, even striking and guarding; a few levels up
// (level 5) it wins only actively, at a death every two or three kills. The zone's own set
// at level 1 makes active play easy; by level 5 the auto attack farms it idle while dying
// often, by level 10 the deaths are rare and by level 25 gone. Defense is what keeps the
// level-1 kit out: it sits just under that kit's strike and the own set's level-5 attack,
// so the fight is a race the weaker kit loses. Attack sits near half the own set's
// level-25 defense (hits lose half the defense, see hurtThrough), and health sets how long
// an idle kill takes. A zone's boss is its hardest enemy.
//
// The run the tables are tuned for (tools/sim, a player checking in hourly while awake):
// every zone but the Sewers opens on its map (GameConfig.h): Old Mine at 0.5 h, Forest at
// 2 h, Barrows at 4-5 h, Crypt at 8 h, Onyx Caves at 14-15 h, Depths at 24-26 h, Molten
// Core at 48 h. The sim prints, per zone, the awake hours to farming itself and to farming
// without dying. Ascending when the game suggests it, the first run ends at Depths around
// day 1.
//
// The two zones past the Core are for later runs, opened by Deep Delver (see
// AscensionConfig.h for when); they are not on the ladder yet. The Rift farms itself
// within hours of opening, the Core set maxed by then. The Forge's defense sits at the
// maxed Mythic attack, so it farms itself within hours, and its health keeps an idle kill
// around half a minute, to farm the Star set, its relic and tools.
//
// The Sewers are set by hand, against the starting kit; every regular sits at defense 10,
// so no level-1 kit leaves one immortal to an idle fight.
//
// Table gold per kill is about 5x the expected gold/s when the zone opens (the Sewers
// excepted, which open before there is any production to speak of), and the zone's
// killSeconds of production come on top, so farming pays a tenth to a fifth of the income
// at any point of the run instead of fading as the buildings grow (about 40% more
// ascension points by day 3 than table gold alone).
static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name           hp      atk   def   interval  gold       sprite                drops
    {"Small Rat",     26,     23,   10,   2200,     12,        &sprite_small_rat,    DROPS(SMALL_RAT_DROPS)},
    {"Slime",         34,     24,   10,   2800,     18,        &sprite_slime,        DROPS(SLIME_DROPS)},
    {"Frog",          30,     24,   10,   2500,     18,        &sprite_frog,         DROPS(FROG_DROPS)},
    {"Sad Frog",      28,     26,   10,   2600,     21,        &sprite_sad_frog,     DROPS(SAD_FROG_DROPS)},
    {"Big Bad Rat",   38,     25,   10,   2400,     27,        &sprite_big_bad_rat,  DROPS(BIG_BAD_RAT_DROPS)},
    {"Frog Wizard",   36,     26,   10,   2800,     30,        &sprite_frog_wizard,  DROPS(FROG_WIZARD_DROPS)},
    {"Gamer Mouse",   55,     29,   12,   2600,     90,        &sprite_gamer_mouse,  DROPS(GAMER_MOUSE_DROPS), true},
};
static constexpr EnemyDef MINE_ENEMIES[] = {
    {"Cave Bat",      99,     39,   35,  2300,     6000,      PLACEHOLDER_SPRITE,   DROPS(CAVE_BAT_DROPS)},
    {"Mole Miner",    108,     41,   36,  2600,     8000,      PLACEHOLDER_SPRITE,   DROPS(MOLE_MINER_DROPS)},
    {"Rock Troll",    135,    40,   37,  2600,     25000,     PLACEHOLDER_SPRITE,   DROPS(ROCK_TROLL_DROPS), true},
};
static constexpr EnemyDef FOREST_ENEMIES[] = {
    {"Spider",        245,    67,   45,      2300,     15000,     &sprite_spider,       DROPS(SPIDER_DROPS)},
    {"Gnome",         250,    68,   46,      2500,     17000,     &sprite_gnome,        DROPS(GNOME_DROPS)},
    {"Bard Gnome",    255,    68,   46,      2600,     19000,     &sprite_bard_gnome,   DROPS(BARD_GNOME_DROPS)},
    {"Goblin",        260,    69,   47,      2400,     22000,     &sprite_goblin,       DROPS(GOBLIN_DROPS)},
    {"Zombie",        265,    70,   47,      2800,     26000,     &sprite_zombie,       DROPS(ZOMBIE_DROPS)},
    {"Gnome Band",    330,    70,   49,      2600,     70000,     &sprite_gnome_band,   DROPS(GNOME_BAND_DROPS), true},
};
static constexpr EnemyDef BARROW_ENEMIES[] = {
    {"Wight",         462,    122,  123,   2400,     200000,    PLACEHOLDER_SPRITE,   DROPS(BARROW_WIGHT_DROPS)},
    {"Grave Hound",   523,    127,  134,   2600,     260000,    PLACEHOLDER_SPRITE,   DROPS(GRAVE_HOUND_DROPS)},
    {"Barrow King",   663,    105,  133,  2600,     800000,    PLACEHOLDER_SPRITE,   DROPS(BARROW_KING_DROPS), true},
};
static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",   569,    351,  138,  2400,     1200000,   PLACEHOLDER_SPRITE,   DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",   660,    361,  150,  2600,     1800000,   PLACEHOLDER_SPRITE,   DROPS(CRYPT_GHOUL_DROPS)},
};
static constexpr EnemyDef ONYX_ENEMIES[] = {
    {"Onyx Beetle",   1192,    623,  426,  2400,     6000000,   PLACEHOLDER_SPRITE,   DROPS(ONYX_BEETLE_DROPS)},
    {"Shade",         1378,   642,  451,  2600,     9000000,   PLACEHOLDER_SPRITE,   DROPS(SHADE_DROPS)},
    {"Onyx Drake",    1736,   633,  457,  2800,     25000000,  PLACEHOLDER_SPRITE,   DROPS(ONYX_DRAKE_DROPS), true},
};
static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",   2639,   1057, 553,  2400,     20000000,  PLACEHOLDER_SPRITE,   DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",    3042,   1091, 597,  2600,     32000000,  PLACEHOLDER_SPRITE,   DROPS(ABYSS_WORM_DROPS)},
};
static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",   4152,   1816, 1262, 2400,     480000000, PLACEHOLDER_SPRITE,   DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",    4789,   1864, 1330, 2600,     720000000, PLACEHOLDER_SPRITE,   DROPS(CORE_GOLEM_DROPS)},
};
static constexpr EnemyDef RIFT_ENEMIES[] = {
    {"Riftstalker",   4500,   3900, 2600, 2400,     1000000000ULL,   PLACEHOLDER_SPRITE, DROPS(RIFT_STALKER_DROPS)},
    {"Void Wraith",   4800,   4000, 2700, 2600,     1400000000ULL,   PLACEHOLDER_SPRITE, DROPS(VOID_WRAITH_DROPS)},
    {"Void Maw",      9000,   4600, 2950, 2800,     4000000000ULL,   PLACEHOLDER_SPRITE, DROPS(VOID_MAW_DROPS), true},
};
static constexpr EnemyDef STAR_ENEMIES[] = {
    {"Star Smith",    16000,  9700, 7300, 2400,     6000000000ULL,   PLACEHOLDER_SPRITE, DROPS(STAR_SMITH_DROPS)},
    {"Nova Golem",    18000,  9900, 7500, 2600,     8000000000ULL,   PLACEHOLDER_SPRITE, DROPS(NOVA_GOLEM_DROPS)},
    {"Sun Eater",     30000,  10400, 7900, 2800,    24000000000ULL,  PLACEHOLDER_SPRITE, DROPS(SUN_EATER_DROPS), true},
};

#define ZONE(table) table, sizeof(table) / sizeof(table[0])

// Zone indices line up with the bonusPercent of the UNLOCK_ZONE upgrades in GameConfig.h
static constexpr ZoneDef ZONES[] = {
    // Only the Sewers have a scene of their own yet; the rest stand on plain stone
    //  name           enemies                needsUpgrade  killSec  icon                               background
    {"Sewers",      ZONE(SEWER_ENEMIES),    false,        1,       image_zone_sewers_pixels,          &background_sewers},
    {"Old Mine",    ZONE(MINE_ENEMIES),     true,         1,       image_building_minecart_pixels,    &background_stone},
    {"Forest",      ZONE(FOREST_ENEMIES),   true,         1,       image_item_fig_leaf_pixels,        &background_stone},
    {"Barrows",     ZONE(BARROW_ENEMIES),   true,         1,       image_award_skull_pixels,          &background_stone},
    {"Crypt",       ZONE(CRYPT_ENEMIES),    true,         2,       image_zone_crypt_pixels,           &background_stone},
    {"Onyx Caves",  ZONE(ONYX_ENEMIES),     true,         2,       image_building_deep_shaft_pixels,  &background_stone},
    {"Depths",      ZONE(DEPTH_ENEMIES),    true,         2,       image_zone_depths_pixels,          &background_stone},
    {"Molten Core", ZONE(CORE_ENEMIES),     true,         2,       image_zone_core_pixels,            &background_stone},
    // Past the Molten Core, opened by the Deep Delver talent instead of a map
    {"Void Rift",   ZONE(RIFT_ENEMIES),     false,        3,       image_icon_zone_void_rift_pixels,  &background_stone, 1},
    {"Star Forge",  ZONE(STAR_ENEMIES),     false,        4,       image_icon_zone_star_forge_pixels, &background_stone, 2},
};
static const uint8_t ZONE_MOLTEN_CORE = 7;
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

// A zone's set is the gear it drops (isSetPiece: no tools, jewellery or relics)
static constexpr bool dropsItem(const DropDef *drops, uint8_t count, uint8_t itemId)
{
    return count > 0 && (drops[0].itemId == itemId || dropsItem(drops + 1, count - 1, itemId));
}
static constexpr bool enemiesDropItem(const EnemyDef *enemies, uint8_t count, uint8_t itemId)
{
    return count > 0 && (dropsItem(enemies[0].drops, enemies[0].dropCount, itemId) ||
                         enemiesDropItem(enemies + 1, count - 1, itemId));
}
static constexpr bool isZoneSetPiece(uint8_t zoneId, uint8_t itemId)
{
    return isSetPiece(ITEMS[itemId]) && enemiesDropItem(ZONES[zoneId].enemies, ZONES[zoneId].enemyCount, itemId);
}
static constexpr uint8_t zoneSetSize(uint8_t zoneId, uint8_t i = 0)
{
    return i >= ITEM_COUNT ? 0 : isZoneSetPiece(zoneId, i) + zoneSetSize(zoneId, i + 1);
}

#undef PLACEHOLDER_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
