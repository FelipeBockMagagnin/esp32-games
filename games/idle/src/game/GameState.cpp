#include "GameState.h"
#include "Inventory.h"
#include "Achievements.h"
#include "Ascension.h"

GameState::GameState(Inventory &inventory, Achievements &achievements, const Ascension &ascension)
    : inventory(inventory),
      achievements(achievements),
      ascension(ascension),
      gold(0),
      productionRemainder(0),
      miningLevel(1),
      miningXp(0),
      totalClicks(0),
      totalGoldEarned(0),
      started(false),
      lastUpdate(0),
      seenAscensionRevision(0),
      lastMineAt(0),
      comboCount(0),
      veinVisible(false),
      veinAt(0),
      buff(VeinEffect::NONE),
      buffUntil(0),
      lastLuckyGold(0),
      veinsCaught(0),
      veinSpawns(0)
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
        scheduleVein(now);
        return;
    }

    // A Heirloom level bought mid-run hands over its buildings at once
    if (ascension.getRevision() != seenAscensionRevision)
    {
        seenAscensionRevision = ascension.getRevision();
        applyHeadStart();
    }

    updateVeins(now);

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

uint32_t GameState::mine(bool &leveledUp, unsigned long now)
{
    bool streak = comboCount > 0 && now - lastMineAt <= COMBO_WINDOW_MS;
    comboCount = streak ? (comboCount < UINT16_MAX ? comboCount + 1 : comboCount) : 1;
    lastMineAt = now;

    uint64_t gained = (uint64_t)getClickAmount() * (100 + getComboPercent(now)) / 100;
    if (gained > UINT32_MAX)
    {
        gained = UINT32_MAX;
    }
    creditGold(gained * GOLD_SCALE);
    totalClicks++;

    leveledUp = false;
    miningXp += getXpPerClick();
    while (miningXp >= getXpToNextLevel())
    {
        miningXp -= getXpToNextLevel();
        miningLevel++;
        leveledUp = true;
    }
    return (uint32_t)gained;
}

// The combo's bonus on the next click: a step per click in the streak beyond the first,
// up to the Combo talent's ceiling, gone once the window since the last click has passed
uint32_t GameState::getComboPercent(unsigned long now) const
{
    uint32_t ceiling = ascension.getPercent(TalentEffect::CLICK_COMBO);
    if (ceiling == 0 || comboCount == 0 || now - lastMineAt > COMBO_WINDOW_MS)
    {
        return 0;
    }
    uint32_t percent = (comboCount - 1) * COMBO_STEP_PERCENT;
    return percent < ceiling ? percent : ceiling;
}

uint32_t GameState::getXpPerClick() const
{
    return MINE_XP + ascension.getPercent(TalentEffect::MINING_XP);
}

// Raises the first buildings to Heirloom's floor; never takes any away
void GameState::applyHeadStart()
{
    uint32_t level = ascension.getPercent(TalentEffect::HEAD_START);
    for (uint8_t i = 0; i < HEAD_START_BUILDINGS && i < BUILDING_COUNT; i++)
    {
        uint16_t floor = HEAD_START_LEVELS[i] * level;
        if (buildingLevels[i] < floor)
        {
            buildingLevels[i] = floor;
        }
    }
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

    gold -= getGoldUpgradeCost(id) * GOLD_SCALE;
    goldUpgradesBought[id] = true;
    return true;
}

void GameState::addGold(uint64_t amount)
{
    creditGold(amount * GOLD_SCALE);
}

// Saturates rather than wrapping: a 64-bit balance in thousandths tops out near 1.8e16 gold
void GameState::creditGold(uint64_t scaledAmount)
{
    gold = gold > UINT64_MAX - scaledAmount ? UINT64_MAX : gold + scaledAmount;
    totalGoldEarned = totalGoldEarned > UINT64_MAX - scaledAmount ? UINT64_MAX : totalGoldEarned + scaledAmount;
}

uint64_t GameState::getGold() const
{
    return gold / GOLD_SCALE;
}

// A running frenzy multiplies everything the buildings make, on screen as well
uint64_t GameState::getProductionPerSecond() const
{
    uint64_t base = getBaseProductionPerSecond();
    return buff == VeinEffect::FRENZY ? base * VEIN_FRENZY_MULTIPLIER : base;
}

uint64_t GameState::getBaseProductionPerSecond() const
{
    uint8_t boosts[BUILDING_COUNT];
    countBoosts(boosts);
    uint64_t total = 0;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        total += getBuildingBaseOutput(i, boosts) * buildingLevels[i];
    }
    return applyProductionBonus(total);
}

// One level's output before the global bonuses: its boosts, then Synergy's share per
// level of the building before it
uint64_t GameState::getBuildingBaseOutput(uint8_t id, const uint8_t *boosts) const
{
    uint64_t output = (uint64_t)BUILDINGS[id].productionPerLevel << boosts[id];
    uint32_t synergy = ascension.getPercent(TalentEffect::SYNERGY);
    if (id > 0 && synergy > 0)
    {
        output = output * (100 + synergy * buildingLevels[id - 1]) / 100;
    }
    return output;
}

// The ascension's share multiplies the rest rather than adding to it, so it stays worth
// as much late in a run, when the upgrades and awards have stacked up, as early on
uint64_t GameState::applyProductionBonus(uint64_t production) const
{
    production = production * (100 + getProductionBonusPercent()) / 100;
    return production * (100 + getAscensionProductionPercent()) / 100;
}

uint32_t GameState::getAscensionProductionPercent() const
{
    return ascension.getPointsEarned() * ASCEND_PRODUCTION_PER_POINT + ascension.getPercent(TalentEffect::PRODUCTION) +
           ascension.getPercent(TalentEffect::PRODUCTION_PER_AWARD) * achievements.getUnlockedCount();
}

// Bought BUILDING_BOOST upgrades per building; each one doubles that building's output
void GameState::countBoosts(uint8_t *boosts) const
{
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        boosts[i] = 0;
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        const GoldUpgradeDef &def = GOLD_UPGRADES[i];
        if (goldUpgradesBought[i] && def.target == UpgradeTarget::BUILDING_BOOST && def.reqBuilding < BUILDING_COUNT)
        {
            boosts[def.reqBuilding]++;
        }
    }
}

uint32_t GameState::getBuildingMultiplier(uint8_t id) const
{
    uint8_t boosts[BUILDING_COUNT];
    countBoosts(boosts);
    return id < BUILDING_COUNT ? (uint32_t)1 << boosts[id] : 1;
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
    uint8_t boosts[BUILDING_COUNT];
    countBoosts(boosts);
    return applyProductionBonus(getBuildingBaseOutput(id, boosts));
}

uint32_t GameState::getClickAmount() const
{
    uint64_t amount = ORE_TIERS[getOreTier()].clickAmount;
    amount = amount * (100 + getClickBonusPercent()) / 100;
    amount += getProductionPerSecond() * getClickProductionPercent() / 100 / GOLD_SCALE;
    if (buff == VeinEffect::CLICK_FRENZY)
    {
        amount *= VEIN_CLICK_MULTIPLIER;
    }
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
    uint32_t discount = ascension.getPercent(TalentEffect::BUILDING_DISCOUNT);
    return discount >= 100 ? 0 : cost - cost * discount / 100;
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
    // A map Old Maps keeps is never on sale
    if (def.target == UpgradeTarget::UNLOCK_ZONE && def.bonusPercent <= ascension.getPercent(TalentEffect::KEEP_MAPS))
    {
        return false;
    }
    if (def.reqBuilding != NO_BUILDING_REQ && getBuildingLevel(def.reqBuilding) < def.reqBuildingLevel)
    {
        return false;
    }
    return miningLevel >= def.reqMiningLevel;
}

bool GameState::canAffordGoldUpgrade(uint8_t id) const
{
    return isGoldUpgradeUnlocked(id) && !goldUpgradesBought[id] && getGold() >= getGoldUpgradeCost(id);
}

uint64_t GameState::getGoldUpgradeCost(uint8_t id) const
{
    if (id >= GOLD_UPGRADE_COUNT)
    {
        return 0;
    }
    uint32_t discount = inventory.getRelicPercent(RelicEffect::UPGRADE_DISCOUNT);
    uint64_t cost = GOLD_UPGRADES[id].cost;
    return discount >= 100 ? 0 : cost - cost * discount / 100;
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
           achievements.getProductionBonusPercent() * (100 + inventory.getRelicPercent(RelicEffect::AWARD_BOOST)) / 100;
}

uint32_t GameState::getClickBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::CLICK) + inventory.getClickBonusPercent() +
           ascension.getPercent(TalentEffect::CLICK);
}

uint32_t GameState::getClickProductionPercent() const
{
    return getBonusPercent(UpgradeTarget::CLICK_PRODUCTION);
}

uint32_t GameState::getVeinFrequencyPercent() const
{
    return getBonusPercent(UpgradeTarget::VEIN_FREQUENCY) + ascension.getPercent(TalentEffect::VEIN_FREQUENCY);
}

uint32_t GameState::getVeinPowerPercent() const
{
    return getBonusPercent(UpgradeTarget::VEIN_POWER) + ascension.getPercent(TalentEffect::VEIN_POWER);
}

static bool veinReached(unsigned long now, unsigned long deadline)
{
    return (long)(now - deadline) >= 0;
}

// The wait for the next vein is random within the base range, shortened by the frequency
// upgrades
void GameState::scheduleVein(unsigned long now)
{
    unsigned long wait = (unsigned long)random((long)VEIN_MIN_INTERVAL_MS, (long)VEIN_MAX_INTERVAL_MS + 1);
    veinAt = now + wait * 100 / (100 + getVeinFrequencyPercent());
}

void GameState::updateVeins(unsigned long now)
{
    if (buff != VeinEffect::NONE && veinReached(now, buffUntil))
    {
        buff = VeinEffect::NONE;
    }

    if (veinVisible)
    {
        if (veinReached(now, veinAt))
        {
            veinVisible = false; // Missed
            scheduleVein(now);
        }
    }
    else if (veinReached(now, veinAt))
    {
        veinVisible = true;
        veinAt = now + VEIN_VISIBLE_MS + ascension.getPercent(TalentEffect::VEIN_LINGER) * 1000UL;
        veinSpawns++;
    }
}

unsigned long GameState::getVeinRemaining(unsigned long now) const
{
    return veinVisible && !veinReached(now, veinAt) ? veinAt - now : 0;
}

unsigned long GameState::getBuffRemaining(unsigned long now) const
{
    return buff != VeinEffect::NONE && !veinReached(now, buffUntil) ? buffUntil - now : 0;
}

// A caught vein rolls its bonus. A new frenzy replaces one still running rather than
// stacking, so catching veins back to back never multiplies past one bonus.
VeinEffect GameState::catchVein(unsigned long now)
{
    if (!veinVisible)
    {
        return VeinEffect::NONE;
    }
    veinVisible = false;
    veinsCaught++;
    scheduleVein(now);

    uint32_t power = 100 + getVeinPowerPercent();
    long roll = random(100);
    if (roll < VEIN_LUCKY_WEIGHT)
    {
        // A share of the balance, capped by minutes of production, so it pays to bank
        // gold before catching one but never more than the cap
        uint64_t fromBank = gold * VEIN_LUCKY_BANK_PERCENT / 100;
        uint64_t cap = getBaseProductionPerSecond() * VEIN_LUCKY_SECONDS * power / 100;
        uint64_t scaled = (fromBank < cap ? fromBank : cap) + (uint64_t)getClickAmount() * VEIN_LUCKY_CLICKS * GOLD_SCALE;
        creditGold(scaled);
        lastLuckyGold = scaled / GOLD_SCALE;
        return VeinEffect::LUCKY;
    }

    bool frenzy = roll < VEIN_LUCKY_WEIGHT + VEIN_FRENZY_WEIGHT;
    buff = frenzy ? VeinEffect::FRENZY : VeinEffect::CLICK_FRENZY;
    buffUntil = now + (frenzy ? VEIN_FRENZY_MS : VEIN_CLICK_MS) * power / 100;
    return buff;
}

uint32_t GameState::getAttackBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::ATTACK) + achievements.getRewardPercent(AchReward::ATTACK_DEFENSE) +
           ascension.getPercent(TalentEffect::ATTACK_DEFENSE);
}

uint32_t GameState::getDefenseBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::DEFENSE) + achievements.getRewardPercent(AchReward::ATTACK_DEFENSE) +
           ascension.getPercent(TalentEffect::ATTACK_DEFENSE);
}

uint32_t GameState::getMaxHpBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::MAX_HP) + ascension.getPercent(TalentEffect::MAX_HP);
}

bool GameState::isZoneUnlockedByUpgrade(uint8_t zoneId) const
{
    if (zoneId <= ascension.getPercent(TalentEffect::KEEP_MAPS))
    {
        return true;
    }
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
    out.veinsCaught = veinsCaught;
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
    veinsCaught = in.veinsCaught;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        buildingLevels[i] = in.buildingLevels[i];
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        goldUpgradesBought[i] = in.goldUpgradesBought[i];
    }

    applyHeadStart();

    // Force update() to re-baseline its clock instead of crediting the gap since boot
    started = false;
}
