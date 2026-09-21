#include "GameState.h"

static uint8_t idx(OreEnum ore)
{
    return static_cast<uint8_t>(ore);
}

GameState::GameState()
    : currentOre(OreEnum::COPPER),
      miningLevel(1),
      miningXp(0),
      started(false),
      lastUpdate(0)
{
    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        ore[i] = 0;
        productionRemainder[i] = 0;
    }
    for (uint8_t i = 0; i < UPGRADE_COUNT; i++)
    {
        upgradeLevels[i] = 0;
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

    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        uint32_t perSecond = getProductionPerSecond(static_cast<OreEnum>(i));
        if (perSecond == 0)
        {
            continue;
        }

        // perSecond is ORE_SCALE units per 1000 ms, so keep the sub-unit remainder between frames
        uint64_t scaled = (uint64_t)perSecond * elapsed + productionRemainder[i];
        ore[i] += scaled / 1000;
        productionRemainder[i] = scaled % 1000;
    }
}

bool GameState::mine()
{
    addOre(currentOre, MINE_AMOUNT);

    bool leveledUp = false;
    miningXp += MINE_XP;
    while (miningXp >= getXpToNextLevel())
    {
        miningXp -= getXpToNextLevel();
        miningLevel++;
        leveledUp = true;
    }
    return leveledUp;
}

bool GameState::buyUpgrade(uint8_t id)
{
    if (id >= UPGRADE_COUNT || !canAfford(id))
    {
        return false;
    }

    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        ore[i] -= getUpgradeCost(id, static_cast<OreEnum>(i)) * ORE_SCALE;
    }
    upgradeLevels[id]++;
    return true;
}

uint64_t GameState::getOre(OreEnum type) const
{
    return ore[idx(type)] / ORE_SCALE;
}

uint32_t GameState::getProductionPerSecond(OreEnum type) const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < UPGRADE_COUNT; i++)
    {
        if (UPGRADES[i].producedOre == type)
        {
            total += UPGRADES[i].productionPerLevel * upgradeLevels[i];
        }
    }
    return total;
}

uint16_t GameState::getUpgradeLevel(uint8_t id) const
{
    return id < UPGRADE_COUNT ? upgradeLevels[id] : 0;
}

uint64_t GameState::getUpgradeCost(uint8_t id, OreEnum type) const
{
    if (id >= UPGRADE_COUNT)
    {
        return 0;
    }

    const UpgradeDef &def = UPGRADES[id];
    uint64_t cost = def.baseCost[idx(type)];
    for (uint16_t level = 0; level < upgradeLevels[id]; level++)
    {
        if (cost > UINT64_MAX / ORE_SCALE / def.costGrowthPercent)
        {
            return UINT64_MAX / ORE_SCALE; // Too expensive to ever afford
        }
        cost = cost * def.costGrowthPercent / 100;
    }
    return cost;
}

bool GameState::canAfford(uint8_t id) const
{
    if (id >= UPGRADE_COUNT)
    {
        return false;
    }

    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        if (getOre(static_cast<OreEnum>(i)) < getUpgradeCost(id, static_cast<OreEnum>(i)))
        {
            return false;
        }
    }
    return true;
}

void GameState::addOre(OreEnum type, uint64_t amount)
{
    ore[idx(type)] += amount;
}
