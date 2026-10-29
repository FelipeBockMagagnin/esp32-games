#ifndef CRAWL_CONFIG_H
#define CRAWL_CONFIG_H

#include <stdint.h>

static const uint8_t MAP_W = 32;
static const uint8_t MAP_H = 32;

// A map cell: 0 is floor, anything else a wall of that look. The renderer maps each to a texture.
enum Cell : uint8_t
{
    CELL_FLOOR,
    CELL_CRYPT,
    CELL_STONE,
    CELL_SEWER,
    CELL_BONE,
    CELL_BLOOD,
    CELL_PORTAL,
    CELL_COUNT
};

// Each band of floors has its main wall and an accent sprinkled through it
struct WallSet
{
    uint8_t firstFloor;
    uint8_t main;
    uint8_t accent;
};
static constexpr WallSet WALL_SETS[] = {
    {1, CELL_CRYPT, CELL_STONE},
    {4, CELL_SEWER, CELL_STONE},
    {7, CELL_BONE, CELL_CRYPT},
    {10, CELL_BLOOD, CELL_BONE},
};
static constexpr uint8_t WALL_SET_COUNT = sizeof(WALL_SETS) / sizeof(WALL_SETS[0]);

// Every BOSS_EVERY floors the lich guards a sealed portal
static const uint8_t BOSS_EVERY = 5;

// Speeds in tiles per second, angles in radians, times in ms
static const float PLAYER_RADIUS = 0.22f;
static const float PLAYER_SPEED = 2.4f;
static const float PLAYER_TURN = 2.5f;
static const uint16_t PLAYER_HP = 100;
static const uint16_t PLAYER_MANA = 60;
static const float MANA_REGEN = 3.0f; // Per second, before the ring

// A tap of Confirm shorter than this swings; held longer, it charges a spell
static const uint16_t TAP_MS = 220;
static const uint16_t CHARGE_MS = 1200;
static const uint16_t SWING_MS = 380;
static const float SWING_RANGE = 1.35f;
static const float SWING_ARC = 0.62f; // Half the arc, about 35 degrees
static const uint16_t FIST_DAMAGE = 3;
static const uint8_t SPELL_COST = 14;
static const uint16_t SPELL_BASE = 6; // Damage with no focus
static const float SPELL_SPEED = 6.5f;
static const float BOLT_SPEED = 4.0f;
static const uint16_t HURT_FLASH_MS = 160;

enum EnemyKind : uint8_t
{
    ENEMY_RAT,
    ENEMY_SLIME,
    ENEMY_ZOMBIE,
    ENEMY_GOBLIN,
    ENEMY_SPIDER,
    ENEMY_FROG_WIZARD,
    ENEMY_BIG_RAT,
    ENEMY_SKELETON,
    ENEMY_WRAITH,
    ENEMY_LICH,
    ENEMY_KIND_COUNT
};

struct EnemyDef
{
    const char *name;
    uint16_t hp;
    uint8_t damage;
    float speed;
    float reach;  // Melee: how close it must be. Ranged: how far it shoots from.
    bool ranged;
    uint16_t windupMs; // From the start of an attack to the blow or the shot
    uint16_t restMs;   // After it, before the next
    float scale;       // Of a wall's height
    uint8_t minFloor;  // 0 never spawns by itself
};

// Indexed by EnemyKind, and in the art's enemy order (convert_art.py ENEMIES)
static constexpr EnemyDef ENEMIES[ENEMY_KIND_COUNT] = {
    {"Rat", 10, 4, 2.2f, 0.8f, false, 260, 500, 0.45f, 1},
    {"Slime", 22, 6, 0.9f, 0.8f, false, 420, 700, 0.5f, 1},
    {"Zombie", 40, 10, 0.8f, 0.9f, false, 520, 800, 0.85f, 2},
    {"Goblin", 28, 8, 1.8f, 0.85f, false, 300, 600, 0.75f, 2},
    {"Spider", 34, 9, 2.0f, 0.9f, false, 320, 650, 0.7f, 3},
    {"Frog mage", 30, 9, 1.0f, 6.0f, true, 600, 1300, 0.75f, 3},
    {"Big rat", 60, 13, 1.6f, 0.95f, false, 380, 700, 0.8f, 4},
    {"Skeleton", 55, 14, 1.4f, 0.95f, false, 420, 700, 0.9f, 5},
    {"Wraith", 50, 13, 1.5f, 6.5f, true, 520, 1100, 0.95f, 7},
    {"Lich", 420, 18, 1.0f, 7.0f, true, 500, 900, 1.05f, 0},
};

// Per floor past the first: enemy health and damage grow by these fractions
static const float ENEMY_HP_GROWTH = 0.18f;
static const float ENEMY_DAMAGE_GROWTH = 0.11f;
static const float SIGHT_RANGE = 9.0f;

enum Slot : uint8_t
{
    SLOT_WEAPON,
    SLOT_FOCUS,
    SLOT_ARMOR,
    SLOT_HELM,
    SLOT_BOOTS,
    SLOT_RING,
    SLOT_AMULET,
    SLOT_COUNT
};

struct SlotDef
{
    const char *name;   // In the inventory
    const char *stat;   // What its value is, after the number
    uint16_t base;      // Value of a tier-0 item on floor 1
    uint16_t perTier;
    uint16_t perFloor;  // In tenths, so the slow stats still climb
    uint8_t series;     // Looks per tier in the art (convert_art.py)
    const char *const *names; // A noun per series
};

static constexpr const char *WEAPON_NAMES[] = {"Blade", "Longsword", "Cleaver"};
static constexpr const char *FOCUS_NAMES[] = {"Staff", "Rod", "Grimoire"};
static constexpr const char *ARMOR_NAMES[] = {"Mail"};
static constexpr const char *HELM_NAMES[] = {"Helm", "Visor"};
static constexpr const char *BOOTS_NAMES[] = {"Boots"};
static constexpr const char *RING_NAMES[] = {"Band", "Ring", "Seal"};
static constexpr const char *AMULET_NAMES[] = {"Charm", "Amulet", "Talisman"};

// Weapon: sword damage. Focus: spell damage. Armor: damage taken off each blow. Helm: max HP.
// Boots: speed in percent. Ring: mana regen in tenths per second. Amulet: critical chance in percent.
static constexpr SlotDef SLOTS[SLOT_COUNT] = {
    {"Weapon", "dmg", 7, 4, 18, 3, WEAPON_NAMES},
    {"Focus", "spell", 10, 6, 25, 3, FOCUS_NAMES},
    {"Armor", "armor", 1, 2, 6, 1, ARMOR_NAMES},
    {"Helm", "hp", 10, 10, 30, 2, HELM_NAMES},
    {"Boots", "% spd", 5, 4, 4, 1, BOOTS_NAMES},
    {"Ring", "mana", 8, 6, 8, 3, RING_NAMES},
    {"Amulet", "% crit", 4, 3, 4, 3, AMULET_NAMES},
};

static const uint8_t TIER_COUNT = 5;
static constexpr const char *TIER_NAMES[TIER_COUNT] = {"Rusty", "Iron", "Grim", "Cursed", "Abyssal"};

static const uint8_t BAG_SIZE = 8;
static const uint8_t MAX_POTIONS = 9;
static const uint16_t POTION_HEAL = 45;
static const uint16_t POTION_MANA = 40;

// Percent chances of a kill dropping something
static const uint8_t DROP_ITEM = 22;
static const uint8_t DROP_POTION = 16;

#endif // CRAWL_CONFIG_H
