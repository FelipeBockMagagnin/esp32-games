#include "Unlocks.h"
#include "SatMath.h"
#include "GameState.h"
#include "Inventory.h"
#include "CombatState.h"
#include "Achievements.h"
#include "Ascension.h"
#include "Format.h"

static const unsigned long CHECK_INTERVAL_MS = 500;
static const unsigned long NOTICE_MS = 4000;

// Buildings owned before the upgrade shop opens, so the player has bought a few and
// learned that screen before the next one appears
static const uint32_t UPGRADES_MIN_BUILDINGS = 3;

// Shown in the header notice, indexed by Feature
static const char *const FEATURE_NAMES[FEATURE_COUNT] = {
    "Buildings", "Upgrade", "Awards", "Zones & Combat", "Inventory", "Ascend"};

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

    case Feature::ASCEND:
    {
        const Ascension &ascension = game.getAscension();
        return ascension.getAscensions() > 0 || ascension.getPendingPoints(game.getTotalGoldEarned()) > 0;
    }
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

// Past 32 bits the bar shows thousandths of the way instead, which it draws the same
static void setProgress(Unlocks::Goal &goal, uint64_t done, uint64_t target)
{
    if (done > target)
    {
        done = target;
    }
    if (target > 0xFFFFFFFFULL)
    {
        goal.done = (uint32_t)mulDiv(done, 1000, target);
        goal.target = 1000;
        return;
    }
    goal.done = (uint32_t)done;
    goal.target = (uint32_t)target;
}

// The locked achievement closest to done, as a percentage, since they count different things;
// -1 when every one is unlocked
static int16_t closestAchievement(const Achievements &achievements, const GameState &game,
                                  const Inventory &inventory, const CombatState &combat, uint32_t &percent)
{
    int16_t best = -1;
    percent = 0;
    for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        if (achievements.isUnlocked(i))
        {
            continue;
        }
        const AchievementDef &def = ACHIEVEMENTS[i];
        uint64_t progress = achievements.getProgress(def, game, inventory, combat);
        uint32_t p = progress >= def.amount ? 100 : (uint32_t)mulDiv(progress, 100, def.amount);
        if (best < 0 || p > percent)
        {
            best = i;
            percent = p;
        }
    }
    return best;
}

static int16_t zoneMapUpgrade(uint8_t zoneId)
{
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (GOLD_UPGRADES[i].target == UpgradeTarget::UNLOCK_ZONE && GOLD_UPGRADES[i].bonusPercent == zoneId)
        {
            return i;
        }
    }
    return -1;
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
    hasGoal = true;
    if (next < 0)
    {
        refreshLateGoal(game, inventory, combat, achievements);
        return;
    }

    char *label = goal.label;
    const size_t size = sizeof(goal.label);

    switch ((Feature)next)
    {
    case Feature::BUILDINGS:
        snprintf(label, size, "Earn %lu gold to open Buildings", (unsigned long)BUILDINGS[0].baseCost);
        setProgress(goal, game.getTotalGoldEarned(), BUILDINGS[0].baseCost);
        break;

    case Feature::UPGRADES:
        if (game.getTotalBuildingLevels() < UPGRADES_MIN_BUILDINGS)
        {
            snprintf(label, size, "Own %lu buildings to open Upgrade", (unsigned long)UPGRADES_MIN_BUILDINGS);
            setProgress(goal, game.getTotalBuildingLevels(), UPGRADES_MIN_BUILDINGS);
        }
        else
        {
            // Measured against the cheapest upgrade on offer, which is what will open it
            uint64_t cheapest = 0;
            for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
            {
                if (game.isGoldUpgradeUnlocked(i) && !game.isGoldUpgradeBought(i) &&
                    (cheapest == 0 || game.getGoldUpgradeCost(i) < cheapest))
                {
                    cheapest = game.getGoldUpgradeCost(i);
                }
            }
            snprintf(label, size, "Save gold to open Upgrade");
            setProgress(goal, game.getGold(), cheapest);
        }
        break;

    case Feature::AWARDS:
    {
        uint32_t percent;
        closestAchievement(achievements, game, inventory, combat, percent);
        snprintf(label, size, "Earn an achievement: Awards");
        setProgress(goal, percent, 100);
        break;
    }

    case Feature::COMBAT:
        snprintf(label, size, "Build a Drill to open Combat");
        setProgress(goal, game.getGold(), game.getBuildingCost(BUILDING_DRILL));
        break;

    case Feature::INVENTORY:
        if (combat.isInZone())
        {
            snprintf(label, size, "Drop loot in combat: Inventory");
        }
        else
        {
            snprintf(label, size, "Enter a zone to fight for loot");
        }
        goal.done = 0;
        goal.target = 0;
        break;

    case Feature::ASCEND:
        snprintf(label, size, "Earn %s gold to open Ascend", formatAmount(ASCEND_GOLD_PER_POINT).c_str());
        setProgress(goal, game.getAscension().getLifetimeGold(game.getTotalGoldEarned()), ASCEND_GOLD_PER_POINT);
        break;
    }
}

// Once every screen is open the bar keeps pointing at the next step, the first that applies
void Unlocks::refreshLateGoal(const GameState &game, const Inventory &inventory, const CombatState &combat,
                              const Achievements &achievements)
{
    char *label = goal.label;
    const size_t size = sizeof(goal.label);

    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        if (game.getBuildingLevel(i) == 0)
        {
            snprintf(label, size, "Buy a %s", BUILDINGS[i].name);
            setProgress(goal, game.getGold(), game.getBuildingCost(i));
            return;
        }
    }

    for (uint8_t z = 1; z < ZONE_COUNT; z++)
    {
        if (combat.isZoneUnlocked(z))
        {
            continue;
        }
        const ZoneDef &zone = ZONES[z];
        if (zone.reqDelveLevel > game.getAscension().getEffectLevel(TalentEffect::DEEP_ZONES))
        {
            break;
        }
        if (zone.requiresUnlockUpgrade && !game.isZoneUnlockedByUpgrade(z))
        {
            int16_t map = zoneMapUpgrade(z);
            if (map >= 0 && game.isGoldUpgradeUnlocked(map))
            {
                snprintf(label, size, "Buy %s to open %s", GOLD_UPGRADES[map].name, zone.name);
                setProgress(goal, game.getGold(), game.getGoldUpgradeCost(map));
                return;
            }
            break;
        }
        snprintf(label, size, "Fight in %s to open %s", ZONES[z - 1].name, zone.name);
        setProgress(goal, combat.getZoneKills(z - 1), zone.reqPrevZoneKills);
        return;
    }

    uint64_t runGold = game.getTotalGoldEarned();
    const Ascension &ascension = game.getAscension();
    if (game.getAscendGainPercent() >= ASCEND_SUGGEST_GAIN_PERCENT)
    {
        snprintf(label, size, "Ascend for +%lu points", (unsigned long)ascension.getPendingPoints(runGold));
        setProgress(goal, 1, 1);
        return;
    }

    uint8_t tier = game.getOreTier();
    if (tier + 1 < ORE_TIER_COUNT)
    {
        const OreTierDef &ore = ORE_TIERS[tier + 1];
        snprintf(label, size, "Mine to Lv %u for %s", (unsigned)ore.minLevel, ore.name);
        setProgress(goal, game.getMiningLevel(), ore.minLevel);
        return;
    }

    uint32_t percent;
    int16_t award = closestAchievement(achievements, game, inventory, combat, percent);
    if (award >= 0 && percent > 0)
    {
        snprintf(label, size, "Award: %s", ACHIEVEMENTS[award].name);
        setProgress(goal, percent, 100);
        return;
    }

    snprintf(label, size, "Next ascension point");
    setProgress(goal, runGold, ascension.getRunGoldForNextPoint(runGold));
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
