#include "GameState.h"

GameState::GameState()
    : gold(0),
      productionRemainder(0),
      miningLevel(1),
      miningXp(0),
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

    uint32_t perSecond = getProductionPerSecond();
    if (perSecond == 0)
    {
        return;
    }

    // perSecond is GOLD_SCALE units per 1000 ms, so keep the sub-unit remainder between frames
    uint64_t scaled = (uint64_t)perSecond * elapsed + productionRemainder;
    gold += scaled / 1000;
    productionRemainder = scaled % 1000;
}

uint32_t GameState::mine(bool &leveledUp)
{
    uint32_t gained = getClickAmount();
    gold += (uint64_t)gained * GOLD_SCALE;

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

    gold -= (uint64_t)GOLD_UPGRADES[id].cost * GOLD_SCALE;
    goldUpgradesBought[id] = true;
    return true;
}

uint64_t GameState::getGold() const
{
    return gold / GOLD_SCALE;
}

uint32_t GameState::getProductionPerSecond() const
{
    uint64_t total = 0;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        total += (uint64_t)BUILDINGS[i].productionPerLevel * buildingLevels[i];
    }
    total = total * (100 + getProductionBonusPercent()) / 100;
    return total > UINT32_MAX ? UINT32_MAX : (uint32_t)total;
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

bool GameState::canAffordGoldUpgrade(uint8_t id) const
{
    return id < GOLD_UPGRADE_COUNT && !goldUpgradesBought[id] && getGold() >= GOLD_UPGRADES[id].cost;
}

uint32_t GameState::getProductionBonusPercent() const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (goldUpgradesBought[i] && GOLD_UPGRADES[i].target == UpgradeTarget::PRODUCTION)
        {
            total += GOLD_UPGRADES[i].bonusPercent;
        }
    }
    return total;
}

uint32_t GameState::getClickBonusPercent() const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (goldUpgradesBought[i] && GOLD_UPGRADES[i].target == UpgradeTarget::CLICK)
        {
            total += GOLD_UPGRADES[i].bonusPercent;
        }
    }
    return total;
}

uint8_t GameState::getOreTier() const
{
    uint16_t tier = (miningLevel - 1) / LEVELS_PER_TIER;
    return tier < ORE_TIER_COUNT ? (uint8_t)tier : ORE_TIER_COUNT - 1;
}
