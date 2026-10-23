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
    WARD   // No damage taken for `power` milliseconds
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
    {ITEM_EMBER_CHARM,   "AEGIS", SpellEffect::WARD,   6000,  16000},
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
// auto-attack through a new zone; the long farm is meant for its levels, not for finding it.
static const uint8_t NEW_ITEM_DROP_MULTIPLIER = 4;

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
    uint16_t reqPrevZoneKills;  // Kills needed in the zone before it; 0 for none
    // Seconds of base production a kill pays on top of its table gold (a boss
    // BOSS_KILL_SECONDS times as many), so fighting keeps its share of the income once the
    // buildings outgrow the table
    uint8_t killSeconds;
    const uint16_t *icon;       // 16x16 RGB565, black transparent
    const SpriteDef *background; // 64x64, drawn behind the enemy at 2x
    uint8_t reqDelveLevel;      // Deep Delver levels needed; left off the first zones' rows
};

// Drop chances are fixed; only equipped items, awards, talents and a first find raise them. A repeat
// drop is one level of a hundred, so a zone's chances are set for its set to be found in
// minutes and stand around level 30-90 when the next zone opens, which keeps the arrival
// stats the enemy tables below are tuned for. They fall from zone to zone, so the early
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

static constexpr DropDef SPIDER_DROPS[] = {
    {ITEM_LEATHER_CAP, 420},
    {ITEM_IRON_GLOVES, 420},
    {ITEM_LOOT_GLOVES, 350},
};
static constexpr DropDef GNOME_DROPS[] = {
    {ITEM_GOOD_STICK, 420},
    {ITEM_IRON_MAIL, 420},
    {ITEM_IRON_PICK, 350},
};
static constexpr DropDef BARD_GNOME_DROPS[] = {
    {ITEM_LOINCLOTH, 420},
    {ITEM_IRON_BOOTS, 420},
    {ITEM_LEATHER_CAP, 420},
    {ITEM_MINER_HELM, 350},
};
static constexpr DropDef GOBLIN_DROPS[] = {
    {ITEM_GOOD_STICK, 420},
    {ITEM_IRON_GLOVES, 420},
    {ITEM_IRON_SHIELD, 250},
};
static constexpr DropDef ZOMBIE_DROPS[] = {
    {ITEM_IRON_MAIL, 420},
    {ITEM_LOINCLOTH, 420},
    {ITEM_IRON_BOOTS, 420},
};
static constexpr DropDef GNOME_BAND_DROPS[] = {
    {ITEM_IRON_SHIELD, 250},
    {ITEM_GOLD_RING, 400},
    {ITEM_JADE_AMULET, 400},
    {ITEM_HAWK_GLOVES, 30},
};

static constexpr DropDef BONE_DIGGER_DROPS[] = {
    {ITEM_STEEL_HELM, 135},
    {ITEM_STEEL_GRIPS, 135},
    {ITEM_GREAT_STICK, 135},
    {ITEM_STEEL_PICK, 80},
    {ITEM_GRAVE_HOOD, 80},
    {ITEM_COIN_RING, 80},
};
static constexpr DropDef CRYPT_GHOUL_DROPS[] = {
    {ITEM_STEEL_PLATE, 135},
    {ITEM_STEEL_LEGS, 135},
    {ITEM_STEEL_BOOTS, 135},
    {ITEM_STEEL_GUARD, 110},
    {ITEM_RUBY_RING, 60},
    {ITEM_GOLD_AMULET, 60},
    {ITEM_RAGE_MASK, 10},
};

static constexpr DropDef DEEP_LURKER_DROPS[] = {
    {ITEM_RUNE_HELM, 30},
    {ITEM_RUNE_GRIPS, 30},
    {ITEM_RUNE_BLADE, 30},
    {ITEM_RUNE_PICK, 16},
};
static constexpr DropDef ABYSS_WORM_DROPS[] = {
    {ITEM_RUNE_PLATE, 30},
    {ITEM_RUNE_LEGS, 30},
    {ITEM_RUNE_BOOTS, 30},
    {ITEM_RUNE_AEGIS, 23},
    {ITEM_SCOUT_BOOTS, 16},
    {ITEM_RUNE_CLEAVE, 6},
};

static constexpr DropDef MAGMA_HOUND_DROPS[] = {
    {ITEM_RUNE_BLADE, 40},
    {ITEM_RUNE_AEGIS, 30},
    {ITEM_MINER_RING, 30},
    {ITEM_GILT_SHIELD, 40},
};
static constexpr DropDef CORE_GOLEM_DROPS[] = {
    {ITEM_RUNE_PLATE, 40},
    {ITEM_RUNE_HELM, 40},
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
    {ITEM_EMBER_HELM, 20},
    {ITEM_EMBER_GRIPS, 20},
    {ITEM_EMBER_BLADE, 20},
    {ITEM_EMBER_GUARD, 16},
    {ITEM_VOID_PICK, 8},
};
static constexpr DropDef NOVA_GOLEM_DROPS[] = {
    {ITEM_EMBER_MAIL, 20},
    {ITEM_EMBER_LEGS, 20},
    {ITEM_EMBER_BOOTS, 20},
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

// Balance rule: each zone is tuned against the stats a player typically arrives with
// (the previous zone's set part-levelled, plus the combat upgrades bought by then),
// measured with tools/sim rather than assumed:
//
//   arrival (atk / def / hp):  Sewers 10/5/90 (starting kit)   Forest 21/30-33/228-234
//                              Crypt 105-115/230-250/468-496  Depths 453-469/1100-1125/2620
//                              Molten Core 1030-1060/1870-1890/4300-4500
//                              Void Rift 2220-2620/2790-3080/5750-5840 (5760-6750 atk by its second run)
//                              Star Forge 7580-7940/8530-9180/16530-16620 (Mythic set maxed)
//
// Defense sits at 1.2-1.5x that attack, so the auto-attack is blocked on arrival while the
// strike gets through. A hit costs 3-5% of max HP (8% for a boss), so a timed guard keeps
// a fresh arrival alive. Health makes an arrival fight last about four or five strikes.
// The zone's own drops are what make it idle-farmable.
//
// The run the tables are tuned for (tools/sim, a player checking in hourly while awake):
// Sewers from minute 5, Forest at 1 h, Crypt at 7-8 h, Depths with the Transmuter at 24 h,
// Molten Core with the Void Pit at 48 h; each zone farms itself within 0-6 awake hours
// of opening (the sim prints it per run). The zone maps and combat upgrades are gated in GameConfig.h
// to keep these hours. The kill gates (150 / 500 / 1500 / 1000) keep a zone from opening
// before the one before it has been fought for a while.
//
// The two zones past the Core are for later runs: Deep Delver's two levels open them around
// days 13.5 and 23.5 of ascending when the game suggests it (tools/sim --suggest --days 10
// --runs 7), so the Rift is the fourth run's goal. It farms itself 1-7 awake hours after it
// first opens, once a few Mythic pieces have dropped, and levels the set to about 100 by the
// time the Star Forge opens. The Forge breaks the 1.2-1.5x rule: its defense sits at that
// maxed attack, so it farms itself within hours, and its health keeps an idle kill around
// half a minute (~115 an hour), to farm its relic and tools.
//
// The Sewers are set by hand: fought with the starting kit, they cost about 40% of health
// per fight and a death every three to four kills until the first drops come in.
//
// The last enemy of the Sewers and the Forest is a boss: met as often as the others, but
// with about twice the health and a hit that a fresh arrival can only survive by guarding.
// Its defense stays a step above the zone's, so the zone's label (set by its hardest
// enemy) is still STRIKE on arrival, and it pays several kills' worth of gold.
//
// Table gold per kill is about 5x the expected gold/s when the zone opens (the Sewers
// excepted, which open before there is any production to speak of), and the zone's
// killSeconds of production come on top, so farming pays a tenth to a fifth of the income
// at any point of the run instead of fading as the buildings grow (about 40% more
// ascension points by day 3 than table gold alone).
static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name           hp      atk   def   interval  gold       sprite                drops
    {"Small Rat",     26,     9,    11,   2200,     12,        &sprite_small_rat,    DROPS(SMALL_RAT_DROPS)},
    {"Slime",         34,     10,   12,   2800,     18,        &sprite_slime,        DROPS(SLIME_DROPS)},
    {"Frog",          30,     11,   12,   2500,     18,        &sprite_frog,         DROPS(FROG_DROPS)},
    {"Sad Frog",      28,     12,   13,   2600,     21,        &sprite_sad_frog,     DROPS(SAD_FROG_DROPS)},
    {"Big Bad Rat",   38,     12,   13,   2400,     27,        &sprite_big_bad_rat,  DROPS(BIG_BAD_RAT_DROPS)},
    {"Frog Wizard",   36,     14,   13,   2800,     30,        &sprite_frog_wizard,  DROPS(FROG_WIZARD_DROPS)},
    {"Gamer Mouse",   55,     13,   15,   2600,     90,        &sprite_gamer_mouse,  DROPS(GAMER_MOUSE_DROPS), true},
};
static constexpr EnemyDef FOREST_ENEMIES[] = {
    {"Spider",        90,     50,   26,   2300,     15000,     &sprite_spider,       DROPS(SPIDER_DROPS)},
    {"Gnome",         93,     52,   27,   2500,     17000,     &sprite_gnome,        DROPS(GNOME_DROPS)},
    {"Bard Gnome",    95,     54,   28,   2600,     19000,     &sprite_bard_gnome,   DROPS(BARD_GNOME_DROPS)},
    {"Goblin",        98,     55,   29,   2400,     22000,     &sprite_goblin,       DROPS(GOBLIN_DROPS)},
    {"Zombie",        105,    57,   30,   2800,     26000,     &sprite_zombie,       DROPS(ZOMBIE_DROPS)},
    {"Gnome Band",    154,    62,   32,   2600,     70000,     &sprite_gnome_band,   DROPS(GNOME_BAND_DROPS), true},
};
static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",   300,    180,  124,  2400,     1200000,   PLACEHOLDER_SPRITE,   DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",   320,    184,  135,  2600,     1800000,   PLACEHOLDER_SPRITE,   DROPS(CRYPT_GHOUL_DROPS)},
};
static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",   1100,   471,  500,  2400,     20000000,  PLACEHOLDER_SPRITE,   DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",    1200,   484,  530,  2600,     32000000,  PLACEHOLDER_SPRITE,   DROPS(ABYSS_WORM_DROPS)},
};
static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",   4200,   2100, 1300, 2400,     480000000, PLACEHOLDER_SPRITE,   DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",    4600,   2150, 1380, 2600,     720000000, PLACEHOLDER_SPRITE,   DROPS(CORE_GOLEM_DROPS)},
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
    //  name           enemies                   needsUpgrade  prevZoneKills  killSec  icon                        background
    {"Sewers",      ZONE(SEWER_ENEMIES),       false,        0,             1,       image_zone_sewers_pixels,   &background_sewers},
    {"Forest",      ZONE(FOREST_ENEMIES),      true,         150,           1,       image_item_fig_leaf_pixels, &background_stone},
    {"Crypt",       ZONE(CRYPT_ENEMIES),       true,         500,           2,       image_zone_crypt_pixels,    &background_stone},
    {"Depths",      ZONE(DEPTH_ENEMIES),       true,         1500,          2,       image_zone_depths_pixels,   &background_stone},
    {"Molten Core", ZONE(CORE_ENEMIES),        true,         1000,          2,       image_zone_core_pixels,     &background_stone},
    // Past the Molten Core, opened by the Deep Delver talent instead of a map
    {"Void Rift",   ZONE(RIFT_ENEMIES),        false,        1000,          3,       image_icon_zone_void_rift_pixels,  &background_stone, 1},
    {"Star Forge",  ZONE(STAR_ENEMIES),        false,        1500,          4,      image_icon_zone_star_forge_pixels, &background_stone, 2},
};
static const uint8_t ZONE_MOLTEN_CORE = 4;
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

#undef PLACEHOLDER_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
