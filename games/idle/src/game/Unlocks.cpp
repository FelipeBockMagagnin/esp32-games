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
      noticeUntil(0)
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
