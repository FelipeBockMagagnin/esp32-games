#include "Achievements.h"
#include "GameState.h"
#include "Inventory.h"
#include "CombatState.h"

// Conditions are thresholds on slow-moving counters, so there is nothing to gain from
// checking them every loop
static const unsigned long CHECK_INTERVAL_MS = 500;

Achievements::Achievements()
    : unlockedCount(0),
      lastCheck(0),
      noticeName(nullptr),
      noticeUntil(0)
{
    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        unlocked[i] = false;
    }
    for (uint8_t i = 0; i < ACH_REWARD_COUNT; i++)
    {
        rewardPercent[i] = 0;
    }
}

bool Achievements::isUnlocked(uint8_t id) const
{
    return id < ACHIEVEMENT_COUNT && unlocked[id];
}

bool Achievements::isConditionMet(const AchievementDef &def, const GameState &game,
                                  const Inventory &inventory, const CombatState &combat) const
{
    return getProgress(def, game, inventory, combat) >= def.amount;
}

uint64_t Achievements::getProgress(const AchievementDef &def, const GameState &game,
                                   const Inventory &inventory, const CombatState &combat) const
{
    switch (def.kind)
    {
    case AchKind::TOTAL_CLICKS:
        return game.getTotalClicks();
    case AchKind::TOTAL_GOLD:
        return game.getTotalGoldEarned();
    case AchKind::TOTAL_BUILDINGS:
        return game.getTotalBuildingLevels();
    case AchKind::BUILDING_LEVEL:
        return game.getBuildingLevel(def.target);
    case AchKind::UPGRADES_BOUGHT:
        return game.getUpgradesBoughtCount();
    case AchKind::MINING_LEVEL:
        return game.getMiningLevel();
    case AchKind::TOTAL_KILLS:
        return combat.getTotalKills();
    case AchKind::ZONE_KILLS:
        return combat.getZoneKills(def.target);
    case AchKind::ITEMS_OWNED:
        return inventory.countOwnedItems();
    case AchKind::ITEM_LEVEL:
        return inventory.getHighestItemLevel();
    case AchKind::ACHIEVEMENTS:
        return unlockedCount;
    case AchKind::VEINS_CAUGHT:
        return game.getVeinsCaught();
    case AchKind::GEAR_SET_MAXED:
    {
        uint8_t maxed = 0;
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            const ItemDef &item = ITEMS[i];
            if (item.tier == def.target && item.slot != EquipSlot::RING && item.slot != EquipSlot::AMULET &&
                inventory.getItemLevel(i) >= ITEM_MAX_LEVEL)
            {
                maxed++;
            }
        }
        return maxed;
    }
    case AchKind::ITEM_MAXED:
        return inventory.getItemLevel(def.target);
    }
    return 0;
}

void Achievements::unlock(uint8_t id, unsigned long now)
{
    unlocked[id] = true;
    unlockedCount++;
    rewardPercent[(uint8_t)ACHIEVEMENTS[id].reward] += ACHIEVEMENTS[id].bonusPercent;

    // Several can land in one pass; the last one takes the header and the list has the rest
    noticeName = ACHIEVEMENTS[id].name;
    noticeUntil = now + ACHIEVEMENT_NOTICE_MS;
}

void Achievements::evaluate(unsigned long now, const GameState &game, const Inventory &inventory,
                            const CombatState &combat)
{
    if (lastCheck != 0 && now - lastCheck < CHECK_INTERVAL_MS)
    {
        return;
    }
    lastCheck = now;

    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        if (!unlocked[i] && isConditionMet(ACHIEVEMENTS[i], game, inventory, combat))
        {
            unlock(i, now);
        }
    }
}

const char *Achievements::getNotice(unsigned long now) const
{
    if (noticeName == nullptr || (long)(now - noticeUntil) >= 0)
    {
        return nullptr;
    }
    return noticeName;
}

void Achievements::save(Snapshot &out) const
{
    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        out.unlocked[i] = unlocked[i];
    }
}

void Achievements::load(const Snapshot &in)
{
    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        unlocked[i] = in.unlocked[i];
    }
    recount();

    // Loading is not earning: no notice for what the player already had
    noticeName = nullptr;
    noticeUntil = 0;
}

void Achievements::recount()
{
    unlockedCount = 0;
    for (uint8_t i = 0; i < ACH_REWARD_COUNT; i++)
    {
        rewardPercent[i] = 0;
    }
    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        if (unlocked[i])
        {
            unlockedCount++;
            rewardPercent[(uint8_t)ACHIEVEMENTS[i].reward] += ACHIEVEMENTS[i].bonusPercent;
        }
    }
}
