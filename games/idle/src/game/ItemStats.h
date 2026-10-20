#ifndef ITEM_STATS_H
#define ITEM_STATS_H

#include <Arduino.h>
#include "ItemConfig.h"

// What an item gives, stat by stat, at any level, and how it reads on screen. Shared by the
// inventory and the combat drop banner.

// Combat stats, then the perks; the respawn cut stays last, out of the row subtitle.
// Labels stay at 11 chars so a label and "+1500%" fit a panel column.
enum StatKind : uint8_t
{
    STAT_ATTACK,
    STAT_DEFENSE,
    STAT_HP,
    STAT_REGEN,
    STAT_GOLD,
    STAT_CLICK,
    STAT_DROP,
    STAT_KILL_GOLD,
    STAT_DOUBLE_MINE,
    STAT_MINE_XP,
    STAT_DISCOUNT,
    STAT_RESPAWN,
    STAT_KIND_COUNT
};

static const char *const STAT_LABELS[STAT_KIND_COUNT] = {
    "Attack", "Defense", "Max HP", "Regen", "Gold/s", "Click gold",
    "Drop rate", "Kill gold", "Double mine", "Double XP", "Upg. cost", "Respawn"};
static const char *const STAT_SHORT_LABELS[STAT_KIND_COUNT] = {
    "atk", "def", "hp", "rgn", "gold", "clk", "drop", "kill", "2xmine", "2xxp", "cost", "resp"};
static const ItemPerk STAT_PERKS[STAT_KIND_COUNT] = {
    ItemPerk::NONE, ItemPerk::NONE, ItemPerk::NONE, ItemPerk::NONE,
    ItemPerk::GOLD, ItemPerk::CLICK, ItemPerk::DROP, ItemPerk::KILL_GOLD,
    ItemPerk::DOUBLE_MINE, ItemPerk::MINE_XP, ItemPerk::UPGRADE_DISCOUNT, ItemPerk::RESPAWN_CUT};

inline bool isPercentStat(StatKind kind)
{
    return kind >= STAT_GOLD;
}

// Cuts read as what they take off, so they come back negative and less is better
inline bool lowerIsBetter(StatKind kind)
{
    return kind == STAT_DISCOUNT || kind == STAT_RESPAWN;
}

inline bool hasStat(const ItemDef &def, StatKind kind)
{
    switch (kind)
    {
    case STAT_ATTACK:
        return def.attack > 0;
    case STAT_DEFENSE:
        return def.defense > 0;
    case STAT_HP:
        return def.maxHp > 0;
    case STAT_REGEN:
        return def.regen > 0;
    default:
        for (const PerkValue &p : def.perks)
        {
            if (p.kind == STAT_PERKS[kind] && p.value > 0)
            {
                return true;
            }
        }
        return false;
    }
}

inline int32_t statAtLevel(const ItemDef &def, StatKind kind, uint8_t level)
{
    switch (kind)
    {
    case STAT_ATTACK:
        return (int32_t)scaleItemStat(def.attack, level);
    case STAT_DEFENSE:
        return (int32_t)scaleItemStat(def.defense, level);
    case STAT_HP:
        return (int32_t)scaleItemStat(def.maxHp, level);
    case STAT_REGEN:
        return (int32_t)scaleItemStat(def.regen, level);
    default:
    {
        if (level == 0)
        {
            return 0;
        }
        int32_t perk = 0;
        for (const PerkValue &p : def.perks)
        {
            if (p.kind == STAT_PERKS[kind])
            {
                perk += isFlatPerk(p.kind) ? p.value : (int32_t)scaleItemStat(p.value, level);
            }
        }
        return lowerIsBetter(kind) ? -perk : perk;
    }
    }
}

// An empty label prints the bare value, for a change like "+1%"
inline void formatStat(char *buf, size_t size, const char *label, StatKind kind, int32_t value, bool forceSign)
{
    const char *gap = label[0] != '\0' ? " " : "";
    const char *sign = value < 0 ? "-" : value > 0 && forceSign ? "+" : "";
    unsigned long magnitude = (unsigned long)(value < 0 ? -value : value);
    if (kind == STAT_REGEN)
    {
        snprintf(buf, size, "%s%s%s%lu.%lu", label, gap, sign, magnitude / 10, magnitude % 10);
    }
    else
    {
        snprintf(buf, size, "%s%s%s%lu%s", label, gap, sign, magnitude, isPercentStat(kind) ? "%" : "");
    }
}

inline uint32_t relicAtLevel(const ItemDef &def, uint8_t level)
{
    return scaleItemStat(def.relicPercent, level);
}

// A relic's one effect at `value`; with `change` only the signed amount, for a level-up
inline void formatRelic(const ItemDef &def, uint32_t value, char *buf, size_t size, bool change = false)
{
    unsigned long v = value;
    switch (def.relic)
    {
    case RelicEffect::UPGRADE_DISCOUNT:
        snprintf(buf, size, change ? "-%lu%%" : "Upgrades -%lu%%", v);
        return;
    case RelicEffect::COOLDOWN_CUT:
        snprintf(buf, size, change ? "-%lu%%" : "Strike/guard cooldown -%lu%%", v);
        return;
    case RelicEffect::AWARD_BOOST:
        snprintf(buf, size, change ? "+%lu%%" : "Award gold bonus +%lu%%", v);
        return;
    case RelicEffect::HP_REGEN:
        snprintf(buf, size, change ? "+%lu.%lu%%" : "Regen %lu.%lu%% max HP/s", v / 10, v % 10);
        return;
    default:
        buf[0] = '\0';
        return;
    }
}

// Levels after `level` until `valueAt` next changes, 0 if it never does again
template <typename ValueAt>
uint8_t levelsToNextGain(ValueAt valueAt, uint8_t level)
{
    int32_t now = valueAt(level);
    for (uint16_t next = level + 1; next <= ITEM_MAX_LEVEL; next++)
    {
        if (valueAt((uint8_t)next) != now)
        {
            return (uint8_t)(next - level);
        }
    }
    return 0;
}

// The level `valueAt` last changed at, up to `level`; 1 if it has not since level 1
template <typename ValueAt>
uint8_t levelOfLastGain(ValueAt valueAt, uint8_t level)
{
    for (uint8_t at = level; at > 1; at--)
    {
        if (valueAt(at) != valueAt((uint8_t)(at - 1)))
        {
            return at;
        }
    }
    return 1;
}

// The levelled value that rises soonest after `level`: a stat, or the relic's effect
struct NextGain
{
    bool found;
    bool relic;
    StatKind kind;
    uint8_t levels;     // After `level`
    uint8_t sinceLevel; // The level that value last rose at
    int32_t change;
};

inline NextGain findNextGain(const ItemDef &def, uint8_t level)
{
    NextGain best = {};
    if (level == 0 || level >= ITEM_MAX_LEVEL)
    {
        return best;
    }
    for (uint8_t k = 0; k < STAT_KIND_COUNT; k++)
    {
        StatKind kind = (StatKind)k;
        if (!hasStat(def, kind))
        {
            continue;
        }
        auto valueAt = [&](uint8_t l) { return statAtLevel(def, kind, l); };
        uint8_t levels = levelsToNextGain(valueAt, level);
        if (levels > 0 && (!best.found || levels < best.levels))
        {
            best = {true, false, kind, levels, levelOfLastGain(valueAt, level),
                    valueAt((uint8_t)(level + levels)) - valueAt(level)};
        }
    }
    if (def.relicPercent > 0)
    {
        auto valueAt = [&](uint8_t l) { return (int32_t)relicAtLevel(def, l); };
        uint8_t levels = levelsToNextGain(valueAt, level);
        if (levels > 0 && (!best.found || levels < best.levels))
        {
            best = {true, true, STAT_KIND_COUNT, levels, levelOfLastGain(valueAt, level),
                    valueAt((uint8_t)(level + levels)) - valueAt(level)};
        }
    }
    return best;
}

// "Attack +1 in 3 lv", with the given labels; a relic's effect reads "Effect -1% ..."
inline void formatNextGain(char *buf, size_t size, const ItemDef &def, const NextGain &gain,
                           const char *const *labels)
{
    char change[16];
    if (gain.relic)
    {
        formatRelic(def, (uint32_t)gain.change, change, sizeof(change), true);
    }
    else
    {
        formatStat(change, sizeof(change), "", gain.kind, gain.change, true);
    }
    const char *label = gain.relic ? "Effect" : labels[gain.kind];
    if (gain.levels == 1)
    {
        snprintf(buf, size, "%s %s next lv", label, change);
    }
    else
    {
        snprintf(buf, size, "%s %s in %u lv", label, change, (unsigned)gain.levels);
    }
}

#endif // ITEM_STATS_H
