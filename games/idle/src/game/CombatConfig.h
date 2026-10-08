#ifndef COMBAT_CONFIG_H
#define COMBAT_CONFIG_H

#include <Arduino.h>
#include "ItemConfig.h"
#include "../assets/Assets.h"
#include "../assets/Sprites.h"

// Combat pacing. The auto-attack runs on its own clock so a fight progresses on any
// screen; the two manual actions are what the player adds by being present.
static const unsigned long AUTO_ATTACK_MS = 2000;
static const unsigned long STRIKE_COOLDOWN_MS = 3000;
static const uint8_t STRIKE_MULTIPLIER = 2; // Manual strike hits this many times as hard

static const unsigned long GUARD_COOLDOWN_MS = 9000;
static const unsigned long GUARD_DURATION_MS = 3000;
static const uint8_t GUARD_DAMAGE_PERCENT = 50; // Incoming damage while guarding
static const unsigned long RESPAWN_MS = 3000; // Long enough to take in a kill and its drop
static const unsigned long REVIVE_MS = 8000; // Health refills over this long after a death

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

struct EnemyDef
{
    const char *name;
    uint32_t maxHp;
    uint32_t attack;
    uint32_t defense;
    unsigned long attackIntervalMs;
    uint32_t goldReward;
    // 64x64 palette + run-length art, drawn at 2x. The Crypt, Depths and Core enemies
    // borrow a Sewers sprite until they get their own; swapping art is a pointer change.
    const SpriteDef *sprite;
    const DropDef *drops;
    uint8_t dropCount;
};

struct ZoneDef
{
    const char *name; // Up to 11 chars to fit a list row title at text size 2
    const EnemyDef *enemies;
    uint8_t enemyCount;
    bool requiresUnlockUpgrade; // Needs the UNLOCK_ZONE upgrade that carries this zone index
    uint16_t reqPrevZoneKills;  // Kills needed in the zone before it; 0 for none
    const uint16_t *icon;       // 16x16 RGB565, black transparent
    const SpriteDef *background; // 64x64, drawn behind the enemy at 2x
};

// Drop chances are set so each item drops about as often per kill as when a zone had two
// enemies: with N enemies picked at random, an item on k of them at chance c drops at
// k*c/N per kill. The boss of each zone carries its ring and amulet.
static constexpr DropDef SMALL_RAT_DROPS[] = {
    {ITEM_DULL_STICK, 1000},
    {ITEM_RAG_GLOVES, 1000},
};
static constexpr DropDef SLIME_DROPS[] = {
    {ITEM_FIG_LEAF, 1000},
    {ITEM_WORN_BOOTS, 1000},
};
static constexpr DropDef FROG_DROPS[] = {
    {ITEM_LEAF_HAT, 1000},
    {ITEM_CLOTH_VEST, 1000},
};
static constexpr DropDef SAD_FROG_DROPS[] = {
    {ITEM_DULL_STICK, 1000},
    {ITEM_FIG_LEAF, 1000},
    {ITEM_WOOD_SHIELD, 500},
};
static constexpr DropDef BIG_BAD_RAT_DROPS[] = {
    {ITEM_RAG_GLOVES, 1000},
    {ITEM_WORN_BOOTS, 1000},
    {ITEM_CLOTH_VEST, 1000},
};
static constexpr DropDef FROG_WIZARD_DROPS[] = {
    {ITEM_LEAF_HAT, 1000},
    {ITEM_WOOD_SHIELD, 500},
    {ITEM_TIN_RING, 300},
};
static constexpr DropDef GAMER_MOUSE_DROPS[] = {
    {ITEM_TIN_RING, 1000},
    {ITEM_WILSON, 1000},
    {ITEM_WOOD_SHIELD, 700},
};

static constexpr DropDef SPIDER_DROPS[] = {
    {ITEM_LEATHER_CAP, 600},
    {ITEM_IRON_GLOVES, 600},
};
static constexpr DropDef GNOME_DROPS[] = {
    {ITEM_GOOD_STICK, 600},
    {ITEM_IRON_MAIL, 600},
};
static constexpr DropDef BARD_GNOME_DROPS[] = {
    {ITEM_LOINCLOTH, 600},
    {ITEM_IRON_BOOTS, 600},
    {ITEM_LEATHER_CAP, 600},
};
static constexpr DropDef GOBLIN_DROPS[] = {
    {ITEM_GOOD_STICK, 600},
    {ITEM_IRON_GLOVES, 600},
    {ITEM_IRON_SHIELD, 450},
};
static constexpr DropDef ZOMBIE_DROPS[] = {
    {ITEM_IRON_MAIL, 600},
    {ITEM_LOINCLOTH, 600},
    {ITEM_IRON_BOOTS, 600},
};
static constexpr DropDef GNOME_BAND_DROPS[] = {
    {ITEM_IRON_SHIELD, 450},
    {ITEM_GOLD_RING, 400},
    {ITEM_JADE_AMULET, 400},
};

static constexpr DropDef BONE_DIGGER_DROPS[] = {
    {ITEM_STEEL_HELM, 400},
    {ITEM_STEEL_GRIPS, 400},
    {ITEM_GREAT_STICK, 400},
};
static constexpr DropDef CRYPT_GHOUL_DROPS[] = {
    {ITEM_STEEL_PLATE, 400},
    {ITEM_STEEL_LEGS, 400},
    {ITEM_STEEL_BOOTS, 400},
    {ITEM_STEEL_GUARD, 300},
    {ITEM_RUBY_RING, 100},
    {ITEM_GOLD_AMULET, 100},
};

static constexpr DropDef DEEP_LURKER_DROPS[] = {
    {ITEM_RUNE_HELM, 400},
    {ITEM_RUNE_GRIPS, 400},
    {ITEM_RUNE_BLADE, 400},
};
static constexpr DropDef ABYSS_WORM_DROPS[] = {
    {ITEM_RUNE_PLATE, 400},
    {ITEM_RUNE_LEGS, 400},
    {ITEM_RUNE_BOOTS, 400},
    {ITEM_RUNE_AEGIS, 300},
};

static constexpr DropDef MAGMA_HOUND_DROPS[] = {
    {ITEM_RUNE_BLADE, 400},
    {ITEM_RUNE_AEGIS, 300},
    {ITEM_MINER_RING, 100},
};
static constexpr DropDef CORE_GOLEM_DROPS[] = {
    {ITEM_RUNE_PLATE, 400},
    {ITEM_RUNE_HELM, 400},
    {ITEM_MINER_RING, 100},
    {ITEM_CORE_SIGIL, 100},
};

// Until the deeper zones get their own art
#define PLACEHOLDER_SPRITE &sprite_big_bad_rat

#define DROPS(table) table, sizeof(table) / sizeof(table[0])

// Balance rule: each zone is tuned against the stats a player typically arrives with
// (the previous zone's full set near the level cap, plus the combat upgrades bought by
// then), measured by simulation rather than assumed:
//
//   arrival (atk / def / hp):  Sewers 10/5/90 (starting kit)   Forest 21/34/119
//                              Crypt 70/162/523   Depths 400/575/1408   Core 1060/2790/3960
//
// Defense sits at 1.2-1.4x that attack, so the auto-attack is blocked on arrival while the
// strike lands for 60-80% of it. Attack costs roughly a tenth of max HP per hit, so a fresh
// arrival dies every four to six kills unless the guard is timed. Health makes an arrival
// fight last 10-18 s. The zone's own drops are what make it idle-farmable.
//
// The Sewers are set by hand: fought with the starting kit, they cost about 40% of health
// per fight and a death every three to four kills until the first drops come in.
//
// The last enemy of the Sewers and the Forest is a boss: met as often as the others, but
// with about twice the health and a hit that a fresh arrival can only survive by guarding.
// Its defense stays a step above the zone's, so the zone's label (set by its hardest
// enemy) is still STRIKE on arrival, and it pays several kills' worth of gold.
//
// Gold per kill is about 5x the expected gold/s when the zone opens, so fighting actively
// adds roughly 40% on top of the buildings at that point and fades as production grows.
static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name             hp      atk   def   interval  gold      sprite                drops
    {"Small Rat",        26,     9,    11,   2200,     4,        &sprite_small_rat,    DROPS(SMALL_RAT_DROPS)},
    {"Slime",            34,     10,   12,   2800,     6,        &sprite_slime,        DROPS(SLIME_DROPS)},
    {"Frog",             30,     11,   12,   2500,     6,        &sprite_frog,         DROPS(FROG_DROPS)},
    {"Sad Frog",         28,     12,   13,   2600,     7,        &sprite_sad_frog,     DROPS(SAD_FROG_DROPS)},
    {"Big Bad Rat",      38,     12,   13,   2400,     9,        &sprite_big_bad_rat,  DROPS(BIG_BAD_RAT_DROPS)},
    {"Frog Wizard",      36,     14,   13,   2800,     10,       &sprite_frog_wizard,  DROPS(FROG_WIZARD_DROPS)},
    {"Gamer Mouse",      55,     13,   15,   2600,     30,       &sprite_gamer_mouse,  DROPS(GAMER_MOUSE_DROPS)},
};
static constexpr EnemyDef FOREST_ENEMIES[] = {
    {"Spider",           70,     48,   25,   2300,     750,      &sprite_spider,       DROPS(SPIDER_DROPS)},
    {"Gnome",            72,     50,   26,   2500,     850,      &sprite_gnome,        DROPS(GNOME_DROPS)},
    {"Bard Gnome",       74,     52,   27,   2600,     950,      &sprite_bard_gnome,   DROPS(BARD_GNOME_DROPS)},
    {"Goblin",           76,     54,   28,   2400,     1100,     &sprite_goblin,       DROPS(GOBLIN_DROPS)},
    {"Zombie",           82,     57,   29,   2800,     1300,     &sprite_zombie,       DROPS(ZOMBIE_DROPS)},
    {"Gnome Band",       120,    60,   30,   2600,     3500,     &sprite_gnome_band,   DROPS(GNOME_BAND_DROPS)},
};
static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",      240,    210,  84,   2400,     30000,    PLACEHOLDER_SPRITE,  DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",      270,    240,  98,   2600,     45000,    PLACEHOLDER_SPRITE,  DROPS(CRYPT_GHOUL_DROPS)},
};
static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",      1400,   690,  480,  2400,     250000,   PLACEHOLDER_SPRITE,  DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",       1500,   740,  560,  2600,     400000,   PLACEHOLDER_SPRITE,  DROPS(ABYSS_WORM_DROPS)},
};
static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",      3600,   3100, 1300, 2400,     4000000,  PLACEHOLDER_SPRITE,  DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",       4000,   3200, 1500, 2600,     6000000,  PLACEHOLDER_SPRITE,  DROPS(CORE_GOLEM_DROPS)},
};

#define ZONE(table) table, sizeof(table) / sizeof(table[0])

// Zone indices line up with the bonusPercent of the UNLOCK_ZONE upgrades in GameConfig.h
static constexpr ZoneDef ZONES[] = {
    // Only the Sewers have a scene of their own yet; the rest stand on plain stone
    //  name           enemies                   needsUpgrade  prevZoneKills  icon                        background
    {"Sewers",      ZONE(SEWER_ENEMIES),       false,        0,             image_zone_sewers_pixels,   &background_sewers},
    {"Forest",      ZONE(FOREST_ENEMIES),      true,         25,            image_item_fig_leaf_pixels, &background_stone},
    {"Crypt",       ZONE(CRYPT_ENEMIES),       true,         50,            image_zone_crypt_pixels,    &background_stone},
    {"Depths",      ZONE(DEPTH_ENEMIES),       true,         100,           image_zone_depths_pixels,   &background_stone},
    {"Molten Core", ZONE(CORE_ENEMIES),        true,         150,           image_zone_core_pixels,     &background_stone},
};
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

#undef PLACEHOLDER_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
