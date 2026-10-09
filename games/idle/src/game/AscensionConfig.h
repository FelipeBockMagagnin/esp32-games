#ifndef ASCENSION_CONFIG_H
#define ASCENSION_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"
#include "../assets/Sprites.h"

// Ascension restarts the run (gold, buildings, upgrades, mining, zone kills) and keeps the
// awards, the items and the talents. Points follow Cookie Clicker's prestige curve: the
// lifetime total is floor(cbrt(lifetime gold / ASCEND_GOLD_PER_POINT)), and ascending
// banks whatever that total has gained, so ascending early never costs points.
// From tools/sim --runs 3: a first run earns its first point within ten hours and 25-28 by
// the end of day three; spent, those bring the second run's Transmuter from 57 h to about
// 25 h, and that run banks 40-55 more. A fourth run reaches the 1.8e16 balance ceiling.
static const uint64_t ASCEND_GOLD_PER_POINT = 1000000000ULL;

// Every point earned, spent or not, also adds this much gold production, as Cookie
// Clicker's prestige levels do. It and the production talents multiply the other bonuses.
static const uint32_t ASCEND_PRODUCTION_PER_POINT = 2;

// Most effects add a percentage; the rest change a mechanic, and their per-level value
// means what the comment says
enum class TalentEffect : uint8_t
{
    PRODUCTION,          // Gold production
    CLICK,               // Gold per click
    BUILDING_DISCOUNT,   // Taken off every building's price
    PRODUCTION_PER_AWARD, // Gold production per unlocked achievement
    ATTACK_DEFENSE,
    MAX_HP,
    DROP_CHANCE,         // Scales every drop's chance, like the Ruby Luck award
    KILL_GOLD,
    VEIN_FREQUENCY,
    VEIN_POWER,
    CLICK_COMBO,         // Ceiling of the click combo, in percent
    MINING_XP,           // Mining XP added to every click
    HEAD_START,          // Multiplies HEAD_START_LEVELS: the buildings every run starts with
    SYNERGY,             // Percent a building's output gains per level of the building before it
    SECOND_WIND,         // Minutes taken off SECOND_WIND_BASE_MS
    TWIN_DROP,           // Percent chance a drop lands twice
    RELIC_HUNT,          // Relics that can drop, in ITEMS order
    KEEP_MAPS,           // Zone maps kept, from the Forest's
    DEEP_ZONES,          // Zones past the Molten Core opened (ZoneDef::reqDelveLevel)
    VEIN_LINGER,         // Seconds a gold vein stays up beyond VEIN_VISIBLE_MS
    COUNT
};
static constexpr uint8_t TALENT_EFFECT_COUNT = (uint8_t)TalentEffect::COUNT;

// Clicks closer together than the window keep the combo going; each one adds a step, up
// to the CLICK_COMBO ceiling, and a pause starts it over
static const unsigned long COMBO_WINDOW_MS = 1000;
static const uint32_t COMBO_STEP_PERCENT = 5;

// Second Wind turns one death into a revive at half health, then waits this long minus
// the talent's minutes before it can again
static const unsigned long SECOND_WIND_BASE_MS = 12UL * 60 * 1000;
static const uint8_t SECOND_WIND_HP_PERCENT = 50;

// Heirloom's starting buildings per level, from the first building on
static constexpr uint16_t HEAD_START_LEVELS[] = {10, 10, 5};
static constexpr uint8_t HEAD_START_BUILDINGS = sizeof(HEAD_START_LEVELS) / sizeof(HEAD_START_LEVELS[0]);

static const uint8_t NO_TALENT = 0xFF;

struct TalentDef
{
    const char *name;        // Up to 11 chars: a list row title at size 2, indented by its depth
    const char *description; // Up to 22 chars: the row subtitle beside the cost
    TalentEffect effect;
    uint16_t perLevel;       // Added to the effect per level, see TalentEffect
    uint8_t maxLevel;
    uint16_t baseCost;       // Points for the first level
    uint16_t costStep;       // Added to the price by each level already bought
    uint8_t parent;          // Needs a level in this talent first; NO_TALENT for a root
    const uint16_t *icon;    // 16x16 RGB565, black transparent
};

// Names for the TALENTS rows, so parents read as names. The order must match the table.
enum TalentId : uint8_t
{
    TALENT_GILDED_SOUL, TALENT_QUICK_HANDS, TALENT_COMBO, TALENT_DEEP_FOCUS, TALENT_HAGGLER,
    TALENT_LEGACY, TALENT_HEIRLOOM, TALENT_SYNERGY,
    TALENT_WAR_SPIRIT, TALENT_IRON_BODY, TALENT_SECOND_WIND, TALENT_KEEN_EYE, TALENT_BOUNTY,
    TALENT_TWIN_DROP, TALENT_RELIC_SENSE, TALENT_OLD_MAPS, TALENT_DEEP_DELVER,
    TALENT_DOWSER, TALENT_VEIN_LORE, TALENT_LINGERING
};

// Shown in this order, so a talent's parent must come before it: the list draws the tree
// by indenting each talent under its parent.
static constexpr TalentDef TALENTS[] = {
    //  name           description              effect                               val  max cost step parent               icon
    {"Gilded Soul", "+10% gold/s per level",   TalentEffect::PRODUCTION,            10,  10, 1,   1,   NO_TALENT,           image_upgrade_production_pixels},
    {"Quick Hands", "+50% gold/click a level", TalentEffect::CLICK,                 50,  5,  1,   1,   TALENT_GILDED_SOUL,  image_upgrade_click_pixels},
    {"Combo",       "Quick clicks combo up",   TalentEffect::CLICK_COMBO,           50,  4,  2,   2,   TALENT_QUICK_HANDS,  image_upgrade_click_pixels},
    {"Deep Focus",  "+1 mining XP a click",    TalentEffect::MINING_XP,             1,   4,  1,   2,   TALENT_QUICK_HANDS,  image_building_pickaxe_pixels},
    {"Haggler",     "Buildings 2% cheaper",    TalentEffect::BUILDING_DISCOUNT,     2,   10, 2,   1,   TALENT_GILDED_SOUL,  image_award_scroll_pixels},
    {"Legacy",      "+1% gold/s per award",    TalentEffect::PRODUCTION_PER_AWARD,  1,   3,  5,   5,   TALENT_HAGGLER,      image_award_trophy_pixels},
    {"Heirloom",    "Start with buildings",    TalentEffect::HEAD_START,            1,   3,  2,   2,   TALENT_HAGGLER,      image_building_minecart_pixels},
    {"Synergy",     "Buildings boost next",    TalentEffect::SYNERGY,               1,   3,  4,   4,   TALENT_HAGGLER,      image_building_excavator_pixels},

    {"War Spirit",  "+10% ATK and DEF a lvl",  TalentEffect::ATTACK_DEFENSE,        10,  10, 1,   1,   NO_TALENT,           image_sword_02b_pixels},
    {"Iron Body",   "+20% max HP per level",   TalentEffect::MAX_HP,                20,  5,  1,   1,   TALENT_WAR_SPIRIT,   image_upgrade_heart_pixels},
    {"Second Wind", "Cheat death sometimes",   TalentEffect::SECOND_WIND,           2,   3,  2,   2,   TALENT_IRON_BODY,    image_upgrade_heart_pixels},
    {"Keen Eye",    "+10% drop chance a lvl",  TalentEffect::DROP_CHANCE,           10,  5,  2,   2,   TALENT_WAR_SPIRIT,   image_item_ruby_ring_pixels},
    {"Bounty",      "+25% kill gold a level",  TalentEffect::KILL_GOLD,             25,  4,  2,   1,   TALENT_KEEN_EYE,     image_item_miner_ring_pixels},
    {"Twin Drop",   "10% drops count twice",   TalentEffect::TWIN_DROP,             10,  5,  2,   1,   TALENT_KEEN_EYE,     image_item_gold_ring_pixels},
    {"Relic Sense", "Relics start to drop",    TalentEffect::RELIC_HUNT,            1,   4,  3,   3,   TALENT_KEEN_EYE,     image_award_skull_pixels},
    {"Old Maps",    "Keep zone maps",          TalentEffect::KEEP_MAPS,             1,   4,  1,   1,   TALENT_WAR_SPIRIT,   image_upgrade_map_pixels},
    {"Deep Delver", "Opens deeper zones",      TalentEffect::DEEP_ZONES,            1,   2,  8,   8,   TALENT_OLD_MAPS,     image_zone_depths_pixels},

    {"Dowser",      "Veins 10% more often",    TalentEffect::VEIN_FREQUENCY,        10,  5,  1,   1,   NO_TALENT,           image_icon_gold_vein_pixels},
    {"Vein Lore",   "Vein bonuses +10% a lvl", TalentEffect::VEIN_POWER,            10,  5,  2,   1,   TALENT_DOWSER,       image_icon_gold_vein_pixels},
    {"Lingering",   "Veins stay 3s longer",    TalentEffect::VEIN_LINGER,           3,   5,  1,   1,   TALENT_DOWSER,       image_icon_gold_vein_pixels},
};
static constexpr uint8_t TALENT_COUNT = sizeof(TALENTS) / sizeof(TALENTS[0]);

#endif // ASCENSION_CONFIG_H
