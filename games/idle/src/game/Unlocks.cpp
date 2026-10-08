#include "Unlocks.h"
#include "GameState.h"
#include "Inventory.h"
#include "CombatState.h"
#include "Achievements.h"

static const unsigned long CHECK_INTERVAL_MS = 500;
static const unsigned long NOTICE_MS = 4000;

// Buildings owned before the upgrade shop opens, so the player has bought a few and
// learned that screen before the next one appears
static const uint32_t UPGRADES_MIN_BUILDINGS = 3;

// Shown in the header notice, indexed by Feature
static const char *const FEATURE_NAMES[FEATURE_COUNT] = {
    "Buildings", "Upgrade", "Awards", "Zones & Combat", "Inventory"};

Unlocks::Unlocks()
    : revision(0),
      lastCheck(0),
      pendingNotice(-1),
      noticeName(nullptr),
      noticeUntil(0),
      goal{"", 0, 0},
      hasGoal(false)
{
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        unlocked[i] = false;
    }
}

bool Unlocks::isConditionMet(Feature feature, const GameState &game, const Inventory &inventory,
                             const CombatState &combat, const Achievements &achievements) const
{
    switch (feature)
    {
    case Feature::BUILDINGS:
        return game.getTotalGoldEarned() >= BUILDINGS[0].baseCost;

    case Feature::UPGRADES:
        if (game.getTotalBuildingLevels() < UPGRADES_MIN_BUILDINGS)
        {
            return false;
        }
        // Opens the first time there is something in the shop the player can buy
        for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
        {
            if (game.isGoldUpgradeUnlocked(i) && !game.isGoldUpgradeBought(i) && game.canAffordGoldUpgrade(i))
            {
                return true;
            }
        }
        return false;

    case Feature::AWARDS:
        return achievements.getUnlockedCount() > 0;

    case Feature::COMBAT:
        return game.getBuildingLevel(BUILDING_DRILL) > 0;

    case Feature::INVENTORY:
        return inventory.countOwnedItems() > Inventory::getStartingKitSize();
    }
    return false;
}

void Unlocks::evaluate(unsigned long now, const GameState &game, const Inventory &inventory,
                       const CombatState &combat, const Achievements &achievements, bool noticeBusy)
{
    if (pendingNotice >= 0 && !noticeBusy)
    {
        noticeName = FEATURE_NAMES[pendingNotice];
        noticeUntil = now + NOTICE_MS;
        pendingNotice = -1;
    }

    if (lastCheck != 0 && now - lastCheck < CHECK_INTERVAL_MS)
    {
        return;
    }
    lastCheck = now;
    refreshGoal(game, inventory, combat, achievements);

    // Only the first locked feature is a candidate, and nothing new opens while a notice is
    // pending or still up, so even a player who already qualifies for several meets them
    // one after another
    if (pendingNotice >= 0 || getNotice(now) != nullptr)
    {
        return;
    }
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        if (unlocked[i])
        {
            continue;
        }
        if (isConditionMet((Feature)i, game, inventory, combat, achievements))
        {
            unlocked[i] = true;
            revision++;
            pendingNotice = (int8_t)i;
        }
        return;
    }
}

// Progress values are capped to 32 bits for the bar; the goals are all small numbers
static uint32_t clampProgress(uint64_t value)
{
    return value > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (uint32_t)value;
}

void Unlocks::refreshGoal(const GameState &game, const Inventory &inventory, const CombatState &combat,
                          const Achievements &achievements)
{
    int8_t next = -1;
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        if (!unlocked[i])
        {
            next = (int8_t)i;
            break;
        }
    }
    hasGoal = next >= 0;
    if (!hasGoal)
    {
        return;
    }

    char *label = goal.label;
    const size_t size = sizeof(goal.label);

    switch ((Feature)next)
    {
    case Feature::BUILDINGS:
        snprintf(label, size, "Earn %lu gold to open Buildings", (unsigned long)BUILDINGS[0].baseCost);
        goal.done = clampProgress(game.getTotalGoldEarned());
        goal.target = (uint32_t)BUILDINGS[0].baseCost;
        break;

    case Feature::UPGRADES:
        if (game.getTotalBuildingLevels() < UPGRADES_MIN_BUILDINGS)
        {
            snprintf(label, size, "Own %lu buildings to open Upgrade", (unsigned long)UPGRADES_MIN_BUILDINGS);
            goal.done = game.getTotalBuildingLevels();
            goal.target = UPGRADES_MIN_BUILDINGS;
        }
        else
        {
            // Measured against the cheapest upgrade on offer, which is what will open it
            uint64_t cheapest = 0;
            for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
            {
                if (game.isGoldUpgradeUnlocked(i) && !game.isGoldUpgradeBought(i) &&
                    (cheapest == 0 || GOLD_UPGRADES[i].cost < cheapest))
                {
                    cheapest = GOLD_UPGRADES[i].cost;
                }
            }
            snprintf(label, size, "Save gold to open Upgrade");
            goal.done = clampProgress(game.getGold());
            goal.target = clampProgress(cheapest);
        }
        break;

    case Feature::AWARDS:
    {
        // Whichever achievement is closest, as a percentage, since they count different things
        uint32_t best = 0;
        for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
        {
            const AchievementDef &def = ACHIEVEMENTS[i];
            uint64_t progress = achievements.getProgress(def, game, inventory, combat);
            uint32_t percent = progress >= def.amount ? 100 : (uint32_t)(progress * 100 / def.amount);
            if (percent > best)
            {
                best = percent;
            }
        }
        snprintf(label, size, "Earn an achievement: Awards");
        goal.done = best;
        goal.target = 100;
        break;
    }

    case Feature::COMBAT:
        snprintf(label, size, "Build a Drill to open Zones");
        goal.done = clampProgress(game.getGold());
        goal.target = clampProgress(game.getBuildingCost(BUILDING_DRILL));
        break;

    case Feature::INVENTORY:
        if (combat.isInZone())
        {
            snprintf(label, size, "Win loot in combat: Inventory");
        }
        else
        {
            snprintf(label, size, "Enter a zone to fight for loot");
        }
        goal.done = 0;
        goal.target = 0;
        break;
    }
}

const char *Unlocks::getNotice(unsigned long now) const
{
    if (noticeName == nullptr || (long)(now - noticeUntil) >= 0)
    {
        return nullptr;
    }
    return noticeName;
}

void Unlocks::save(Snapshot &out) const
{
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        out.unlocked[i] = unlocked[i];
    }
}

void Unlocks::load(const Snapshot &in)
{
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        unlocked[i] = in.unlocked[i];
    }
    revision++;

    // Loading is not earning: no notice for screens the player already had
    pendingNotice = -1;
    noticeName = nullptr;
    noticeUntil = 0;
}
