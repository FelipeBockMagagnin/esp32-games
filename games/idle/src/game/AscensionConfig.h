#ifndef ASCENSION_CONFIG_H
#define ASCENSION_CONFIG_H

#include <Arduino.h>
#include "../assets/Assets.h"
#include "../assets/Sprites.h"

// Ascension restarts the run (gold, buildings, upgrades, mining, zone kills) and keeps the
// awards, the items and the talents. Points follow Cookie Clicker's prestige curve: the
// lifetime total is floor(cbrt(lifetime gold / ASCEND_GOLD_PER_POINT)), and ascending
// banks whatever that total has gained, so ascending early never costs points.
// Tuned for months of play, measured with tools/sim --runs 30 (an ascension every three
// days): 18-29 points a run for the first month, still about 18 by day 90, and Deep Delver's
// zones around days 16 and 31. A run banks fewer points the more came before it, since
// holding steady would need its gold to grow with the square of the points, so the deep
// talent levels stay out of reach for good.
static const uint64_t ASCEND_GOLD_PER_POINT = 4000000000ULL;

// Every point earned, spent or not, also adds this much gold production, as Cookie
// Clicker's prestige levels do. It and the production talents multiply the other bonuses.
static const uint32_t ASCEND_PRODUCTION_PER_POINT = 1;

// Ascending is suggested once the new points would raise that share by this much. Points
// shrink run after run (the cube root), so "until it doubles" would never come round.
static const uint32_t ASCEND_SUGGEST_GAIN_PERCENT = 10;

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
    uint16_t costGrowthPercent; // Each level costs this much of the one before (125 = +25%)
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
//
// Prices grow geometrically, as Cookie Clicker's and NGU's do, so the tree is a sink for
// months of ascensions rather than something a few runs finish: the cheap first levels of
// everything come early, and the deep levels of the percentage roots stay a long-term goal.
static constexpr TalentDef TALENTS[] = {
    //  name           description              effect                               val  max cost grow% parent               icon
    {"Gilded Soul", "+5% gold/s per level",    TalentEffect::PRODUCTION,            5,   50, 1,   125,  NO_TALENT,           image_upgrade_production_pixels},
    {"Quick Hands", "+25% per click a level", TalentEffect::CLICK,                 25,  20, 1,   130,  TALENT_GILDED_SOUL,  image_upgrade_click_pixels},
    {"Combo",       "Quick clicks combo up",   TalentEffect::CLICK_COMBO,           25,  8,  3,   160,  TALENT_QUICK_HANDS,  image_upgrade_click_pixels},
    {"Deep Focus",  "+1 mining XP a click",    TalentEffect::MINING_XP,             1,   5,  2,   180,  TALENT_QUICK_HANDS,  image_building_pickaxe_pixels},
    {"Haggler",     "Buildings 1% cheaper",    TalentEffect::BUILDING_DISCOUNT,     1,   20, 2,   130,  TALENT_GILDED_SOUL,  image_award_scroll_pixels},
    {"Legacy",      "+1% gold/s per award",    TalentEffect::PRODUCTION_PER_AWARD,  1,   5,  10,  200,  TALENT_HAGGLER,      image_award_trophy_pixels},
    {"Heirloom",    "Start with buildings",    TalentEffect::HEAD_START,            1,   5,  3,   200,  TALENT_HAGGLER,      image_building_minecart_pixels},
    {"Synergy",     "Buildings boost next",    TalentEffect::SYNERGY,               1,   5,  15,  250,  TALENT_HAGGLER,      image_building_excavator_pixels},

    {"War Spirit",  "+5% ATK/DEF per level", TalentEffect::ATTACK_DEFENSE,        5,   50, 1,   125,  NO_TALENT,           image_sword_02b_pixels},
    {"Iron Body",   "+10% max HP per level",   TalentEffect::MAX_HP,                10,  30, 1,   125,  TALENT_WAR_SPIRIT,   image_upgrade_heart_pixels},
    {"Second Wind", "Cheat death sometimes",   TalentEffect::SECOND_WIND,           2,   3,  5,   200,  TALENT_IRON_BODY,    image_upgrade_heart_pixels},
    {"Keen Eye",    "+5% drop chance a lvl",   TalentEffect::DROP_CHANCE,           5,   20, 2,   140,  TALENT_WAR_SPIRIT,   image_item_ruby_ring_pixels},
    {"Bounty",      "+10% kill gold a level",  TalentEffect::KILL_GOLD,             10,  20, 2,   140,  TALENT_KEEN_EYE,     image_item_miner_ring_pixels},
    {"Twin Drop",   "5% drops count twice",    TalentEffect::TWIN_DROP,             5,   10, 4,   150,  TALENT_KEEN_EYE,     image_item_gold_ring_pixels},
    {"Relic Sense", "Relics start to drop",    TalentEffect::RELIC_HUNT,            1,   4,  6,   200,  TALENT_KEEN_EYE,     image_award_skull_pixels},
    {"Old Maps",    "Keep zone maps",          TalentEffect::KEEP_MAPS,             1,   4,  3,   150,  TALENT_WAR_SPIRIT,   image_upgrade_map_pixels},
    {"Deep Delver", "Opens deeper zones",      TalentEffect::DEEP_ZONES,            1,   2,  70,  150,  TALENT_OLD_MAPS,     image_zone_depths_pixels},

    {"Dowser",      "Veins 5% more often",     TalentEffect::VEIN_FREQUENCY,        5,   20, 1,   135,  NO_TALENT,           image_icon_gold_vein_pixels},
    {"Vein Lore",   "Vein bonuses +5% a lvl",  TalentEffect::VEIN_POWER,            5,   20, 2,   135,  TALENT_DOWSER,       image_icon_gold_vein_pixels},
    {"Lingering",   "Veins stay 2s longer",    TalentEffect::VEIN_LINGER,           2,   10, 2,   140,  TALENT_DOWSER,       image_icon_gold_vein_pixels},
};
static constexpr uint8_t TALENT_COUNT = sizeof(TALENTS) / sizeof(TALENTS[0]);

#endif // ASCENSION_CONFIG_H
