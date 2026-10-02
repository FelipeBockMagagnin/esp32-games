#include "GameState.h"
#include "Inventory.h"
#include "Achievements.h"

GameState::GameState(Inventory &inventory, Achievements &achievements)
    : inventory(inventory),
      achievements(achievements),
      gold(0),
      productionRemainder(0),
      miningLevel(1),
      miningXp(0),
      totalClicks(0),
      totalGoldEarned(0),
      started(false),
      lastUpdate(0)
{
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        buildingLevels[i] = 0;
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        goldUpgradesBought[i] = false;
    }
}

void GameState::update(unsigned long now)
{
    if (!started)
    {
        started = true;
        lastUpdate = now;
        return;
    }

    unsigned long elapsed = now - lastUpdate; // Unsigned math survives millis() rollover
    if (elapsed == 0)
    {
        return;
    }
    lastUpdate = now;

    uint64_t perSecond = getProductionPerSecond();
    if (perSecond == 0)
    {
        return;
    }

    // perSecond is GOLD_SCALE units per 1000 ms, so keep the sub-unit remainder between frames
    uint64_t scaled = perSecond * elapsed + productionRemainder;
    creditGold(scaled / 1000);
    productionRemainder = scaled % 1000;
}

uint32_t GameState::mine(bool &leveledUp)
{
    uint32_t gained = getClickAmount();
    creditGold((uint64_t)gained * GOLD_SCALE);
    totalClicks++;

    leveledUp = false;
    miningXp += MINE_XP;
    while (miningXp >= getXpToNextLevel())
    {
        miningXp -= getXpToNextLevel();
        miningLevel++;
        leveledUp = true;
    }
    return gained;
}

bool GameState::buyBuilding(uint8_t id)
{
    if (!canAffordBuilding(id))
    {
        return false;
    }

    gold -= getBuildingCost(id) * GOLD_SCALE;
    buildingLevels[id]++;
    return true;
}

bool GameState::buyGoldUpgrade(uint8_t id)
{
    if (!canAffordGoldUpgrade(id))
    {
        return false;
    }

    gold -= GOLD_UPGRADES[id].cost * GOLD_SCALE;
    goldUpgradesBought[id] = true;
    return true;
}

void GameState::addGold(uint64_t amount)
{
    creditGold(amount * GOLD_SCALE);
}

void GameState::creditGold(uint64_t scaledAmount)
{
    gold += scaledAmount;
    totalGoldEarned += scaledAmount;
}

uint64_t GameState::getGold() const
{
    return gold / GOLD_SCALE;
}

uint64_t GameState::getProductionPerSecond() const
{
    uint64_t total = 0;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        total += (uint64_t)BUILDINGS[i].productionPerLevel * buildingLevels[i];
    }
    return total * (100 + getProductionBonusPercent()) / 100;
}

uint64_t GameState::getBuildingProduction(uint8_t id) const
{
    return getBuildingProductionPerLevel(id) * getBuildingLevel(id);
}

uint64_t GameState::getBuildingProductionPerLevel(uint8_t id) const
{
    if (id >= BUILDING_COUNT)
    {
        return 0;
    }
    return (uint64_t)BUILDINGS[id].productionPerLevel * (100 + getProductionBonusPercent()) / 100;
}

uint32_t GameState::getClickAmount() const
{
    uint64_t amount = ORE_TIERS[getOreTier()].clickAmount;
    amount = amount * (100 + getClickBonusPercent()) / 100;
    return amount > UINT32_MAX ? UINT32_MAX : (uint32_t)amount;
}

uint16_t GameState::getBuildingLevel(uint8_t id) const
{
    return id < BUILDING_COUNT ? buildingLevels[id] : 0;
}

uint64_t GameState::getBuildingCost(uint8_t id) const
{
    if (id >= BUILDING_COUNT)
    {
        return 0;
    }

    const BuildingsDef &def = BUILDINGS[id];
    uint64_t cost = def.baseCost;
    for (uint16_t level = 0; level < buildingLevels[id]; level++)
    {
        if (cost > UINT64_MAX / GOLD_SCALE / def.costGrowthPercent)
        {
            return UINT64_MAX / GOLD_SCALE; // Too expensive to ever afford
        }
        cost = cost * def.costGrowthPercent / 100;
    }
    return cost;
}

bool GameState::canAffordBuilding(uint8_t id) const
{
    return id < BUILDING_COUNT && getGold() >= getBuildingCost(id);
}

bool GameState::isGoldUpgradeBought(uint8_t id) const
{
    return id < GOLD_UPGRADE_COUNT && goldUpgradesBought[id];
}

bool GameState::isGoldUpgradeUnlocked(uint8_t id) const
{
    if (id >= GOLD_UPGRADE_COUNT)
    {
        return false;
    }

    const GoldUpgradeDef &def = GOLD_UPGRADES[id];
    if (def.reqBuilding != NO_BUILDING_REQ && getBuildingLevel(def.reqBuilding) < def.reqBuildingLevel)
    {
        return false;
    }
    return miningLevel >= def.reqMiningLevel;
}

bool GameState::canAffordGoldUpgrade(uint8_t id) const
{
    return isGoldUpgradeUnlocked(id) && !goldUpgradesBought[id] && getGold() >= GOLD_UPGRADES[id].cost;
}

uint32_t GameState::getBonusPercent(UpgradeTarget target) const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (goldUpgradesBought[i] && GOLD_UPGRADES[i].target == target)
        {
            total += GOLD_UPGRADES[i].bonusPercent;
        }
    }
    return total;
}

uint32_t GameState::getProductionBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::PRODUCTION) +
           inventory.getGoldProductionBonusPercent() +
           achievements.getProductionBonusPercent();
}

uint32_t GameState::getClickBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::CLICK) + inventory.getClickBonusPercent();
}

uint32_t GameState::getAttackBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::ATTACK);
}

uint32_t GameState::getDefenseBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::DEFENSE);
}

uint32_t GameState::getMaxHpBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::MAX_HP);
}

bool GameState::isZoneUnlockedByUpgrade(uint8_t zoneId) const
{
    // UNLOCK_ZONE upgrades carry the zone index in bonusPercent instead of a percentage
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (goldUpgradesBought[i] &&
            GOLD_UPGRADES[i].target == UpgradeTarget::UNLOCK_ZONE &&
            GOLD_UPGRADES[i].bonusPercent == zoneId)
        {
            return true;
        }
    }
    return false;
}

uint64_t GameState::getTotalGoldEarned() const
{
    return totalGoldEarned / GOLD_SCALE;
}

uint32_t GameState::getTotalBuildingLevels() const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        total += buildingLevels[i];
    }
    return total;
}

uint8_t GameState::getUpgradesBoughtCount() const
{
    uint8_t total = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (goldUpgradesBought[i])
        {
            total++;
        }
    }
    return total;
}

uint8_t GameState::getOreTier() const
{
    uint16_t tier = (miningLevel - 1) / LEVELS_PER_TIER;
    return tier < ORE_TIER_COUNT ? (uint8_t)tier : ORE_TIER_COUNT - 1;
}

void GameState::save(Snapshot &out) const
{
    out.gold = gold;
    out.productionRemainder = productionRemainder;
    out.miningLevel = miningLevel;
    out.miningXp = miningXp;
    out.totalClicks = totalClicks;
    out.totalGoldEarned = totalGoldEarned;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        out.buildingLevels[i] = buildingLevels[i];
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        out.goldUpgradesBought[i] = goldUpgradesBought[i];
    }
}

void GameState::load(const Snapshot &in)
{
    gold = in.gold;
    productionRemainder = in.productionRemainder;
    miningLevel = in.miningLevel > 0 ? in.miningLevel : 1;
    miningXp = in.miningXp;
    totalClicks = in.totalClicks;
    totalGoldEarned = in.totalGoldEarned;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        buildingLevels[i] = in.buildingLevels[i];
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        goldUpgradesBought[i] = in.goldUpgradesBought[i];
    }

    // Force update() to re-baseline its clock instead of crediting the gap since boot
    started = false;
}
