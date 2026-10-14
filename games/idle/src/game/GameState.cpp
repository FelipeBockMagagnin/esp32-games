#include "GameState.h"
#include "Inventory.h"
#include "Achievements.h"
#include "Ascension.h"
#include "SatMath.h"

// No roll at all without the perk, so the random sequence of a run without it is unchanged
static bool rollPercent(uint32_t percent)
{
    return percent > 0 && (uint32_t)random(100) < percent;
}

GameState::GameState(Inventory &inventory, Achievements &achievements, const Ascension &ascension)
    : inventory(inventory),
      achievements(achievements),
      ascension(ascension),
      gold(0),
      goldFraction(0),
      productionRemainder(0),
      miningLevel(1),
      miningXp(0),
      totalClicks(0),
      totalGoldEarned(0),
      totalGoldFraction(0),
      revision(0),
      cachedProductionSignature(UINT32_MAX),
      cachedBaseProduction(0),
      started(false),
      lastUpdate(0),
      seenAscensionRevision(0),
      lastMineAt(0),
      comboCount(0),
      lastMineDoubled(false),
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
    uint64_t scaled = satAdd(satMul(perSecond, elapsed), productionRemainder);
    uint64_t thousandths = scaled / 1000;
    creditGold(thousandths / GOLD_SCALE, (uint32_t)(thousandths % GOLD_SCALE));
    productionRemainder = scaled % 1000;
}

uint64_t GameState::mine(bool &leveledUp, unsigned long now)
{
    bool streak = comboCount > 0 && now - lastMineAt <= COMBO_WINDOW_MS;
    comboCount = streak ? (comboCount < UINT16_MAX ? comboCount + 1 : comboCount) : 1;
    lastMineAt = now;

    uint64_t gained = mulDiv(getClickAmount(), 100 + getComboPercent(now), 100);
    lastMineDoubled = rollPercent(getDoubleMinePercent());
    if (lastMineDoubled)
    {
        gained = satMul(gained, 2);
    }
    creditGold(gained);
    totalClicks++;

    leveledUp = false;
    miningXp += getXpPerClick() * (rollPercent(getDoubleXpPercent()) ? 2 : 1);
    while (miningXp >= getXpToNextLevel())
    {
        miningXp -= getXpToNextLevel();
        miningLevel++;
        leveledUp = true;
    }
    return gained;
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

uint32_t GameState::getDoubleMinePercent() const
{
    uint32_t percent = inventory.getPerkPercent(ItemPerk::DOUBLE_MINE);
    return percent < MAX_DOUBLE_MINE_PERCENT ? percent : MAX_DOUBLE_MINE_PERCENT;
}

uint32_t GameState::getDoubleXpPercent() const
{
    uint32_t percent = inventory.getPerkPercent(ItemPerk::MINE_XP);
    return percent < MAX_MINE_XP_PERCENT ? percent : MAX_MINE_XP_PERCENT;
}

uint32_t GameState::getUpgradeDiscountPercent() const
{
    uint32_t percent = inventory.getRelicPercent(RelicEffect::UPGRADE_DISCOUNT) +
                       inventory.getPerkPercent(ItemPerk::UPGRADE_DISCOUNT);
    return percent < MAX_UPGRADE_DISCOUNT_PERCENT ? percent : MAX_UPGRADE_DISCOUNT_PERCENT;
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
            revision++;
        }
    }
}

bool GameState::buyBuilding(uint8_t id)
{
    if (!canAffordBuilding(id))
    {
        return false;
    }

    gold -= getBuildingCost(id);
    buildingLevels[id]++;
    revision++;
    return true;
}

bool GameState::buyGoldUpgrade(uint8_t id)
{
    if (!canAffordGoldUpgrade(id))
    {
        return false;
    }

    gold -= getGoldUpgradeCost(id);
    goldUpgradesBought[id] = true;
    revision++;
    return true;
}

void GameState::addGold(uint64_t amount)
{
    creditGold(amount);
}

static void addGoldTo(uint64_t &whole, uint16_t &fraction, uint64_t addWhole, uint32_t addThousandths)
{
    uint32_t thousandths = fraction + addThousandths;
    whole = satAdd(satAdd(whole, addWhole), thousandths / GOLD_SCALE);
    fraction = (uint16_t)(thousandths % GOLD_SCALE);
}

// thousandths stays below GOLD_SCALE
void GameState::creditGold(uint64_t whole, uint32_t thousandths)
{
    addGoldTo(gold, goldFraction, whole, thousandths);
    addGoldTo(totalGoldEarned, totalGoldFraction, whole, thousandths);
}

uint64_t GameState::getGold() const
{
    return gold;
}

// A running frenzy multiplies everything the buildings make, on screen as well
uint64_t GameState::getProductionPerSecond() const
{
    uint64_t base = getBaseProductionPerSecond();
    return buff == VeinEffect::FRENZY ? satMul(base, VEIN_FRENZY_MULTIPLIER) : base;
}

// Everything production depends on, as a sum of counters that only grow during a run, so
// any change moves it to a value it never had before. The main loop asks for production
// on every pass; this keeps it from walking the upgrade table each time.
uint32_t GameState::productionSignature() const
{
    return revision + inventory.getRevision() + achievements.getUnlockedCount() + ascension.getRevision();
}

uint64_t GameState::getBaseProductionPerSecond() const
{
    uint32_t signature = productionSignature();
    if (signature == cachedProductionSignature)
    {
        return cachedBaseProduction;
    }

    uint8_t boosts[BUILDING_COUNT];
    countBoosts(boosts);
    uint64_t total = 0;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        total = satAdd(total, satMul(getBuildingBaseOutput(i, boosts), buildingLevels[i]));
    }
    cachedBaseProduction = applyProductionBonus(total);
    cachedProductionSignature = signature;
    return cachedBaseProduction;
}

// One level's output before the global bonuses: its boosts, then Synergy's share per
// level of the building before it
uint64_t GameState::getBuildingBaseOutput(uint8_t id, const uint8_t *boosts) const
{
    uint64_t output = (uint64_t)BUILDINGS[id].productionPerLevel << boosts[id];
    uint32_t synergy = ascension.getPercent(TalentEffect::SYNERGY);
    if (id > 0 && synergy > 0)
    {
        output = mulDiv(output, 100 + synergy * buildingLevels[id - 1], 100);
    }
    return output;
}

// The ascension's share multiplies the rest rather than adding to it, so it stays worth
// as much late in a run, when the upgrades and awards have stacked up, as early on
uint64_t GameState::applyProductionBonus(uint64_t production) const
{
    production = mulDiv(production, 100 + getProductionBonusPercent(), 100);
    return mulDiv(production, 100 + getAscensionProductionPercent(), 100);
}

uint32_t GameState::getAscensionProductionPercent() const
{
    return ascension.getPointsEarned() * ASCEND_PRODUCTION_PER_POINT + ascension.getPercent(TalentEffect::PRODUCTION) +
           ascension.getPercent(TalentEffect::PRODUCTION_PER_AWARD) * achievements.getUnlockedCount();
}

uint32_t GameState::getAscendGainPercent() const
{
    uint64_t gain = (uint64_t)ascension.getPendingPoints(totalGoldEarned) * ASCEND_PRODUCTION_PER_POINT;
    return (uint32_t)mulDiv(gain, 100, 100 + getAscensionProductionPercent());
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
    return satMul(getBuildingProductionPerLevel(id), getBuildingLevel(id));
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

uint64_t GameState::getClickAmount() const
{
    uint64_t amount = mulDiv(ORE_TIERS[getOreTier()].clickAmount, 100 + getClickBonusPercent(), 100);
    amount = satAdd(amount, mulDiv(getProductionPerSecond(), getClickProductionPercent(), 100 * GOLD_SCALE));
    return buff == VeinEffect::CLICK_FRENZY ? satMul(amount, VEIN_CLICK_MULTIPLIER) : amount;
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
        cost = mulDiv(cost, def.costGrowthPercent, 100);
        if (cost == UINT64_MAX)
        {
            return UINT64_MAX; // Past what a balance can hold
        }
    }
    uint32_t discount = ascension.getPercent(TalentEffect::BUILDING_DISCOUNT);
    return discount >= 100 ? 0 : cost - mulDiv(cost, discount, 100);
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
    uint64_t cost = GOLD_UPGRADES[id].cost;
    return cost - mulDiv(cost, getUpgradeDiscountPercent(), 100);
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
           inventory.getPerkPercent(ItemPerk::GOLD) +
           achievements.getProductionBonusPercent() * (100 + inventory.getRelicPercent(RelicEffect::AWARD_BOOST)) / 100;
}

uint32_t GameState::getClickBonusPercent() const
{
    return getBonusPercent(UpgradeTarget::CLICK) + inventory.getPerkPercent(ItemPerk::CLICK) +
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
        uint64_t fromBank = mulDiv(gold, VEIN_LUCKY_BANK_PERCENT, 100);
        uint64_t cap = mulDiv(getBaseProductionPerSecond(), (uint64_t)VEIN_LUCKY_SECONDS * power, 100 * GOLD_SCALE);
        lastLuckyGold = satAdd(fromBank < cap ? fromBank : cap, satMul(getClickAmount(), VEIN_LUCKY_CLICKS));
        creditGold(lastLuckyGold);
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
    return totalGoldEarned;
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
    uint8_t tier = 0;
    while (tier + 1 < ORE_TIER_COUNT && miningLevel >= ORE_TIERS[tier + 1].minLevel)
    {
        tier++;
    }
    return tier;
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
    goldFraction = 0;
    productionRemainder = in.productionRemainder;
    miningLevel = in.miningLevel > 0 ? in.miningLevel : 1;
    miningXp = in.miningXp;
    totalClicks = in.totalClicks;
    totalGoldEarned = in.totalGoldEarned;
    totalGoldFraction = 0;
    veinsCaught = in.veinsCaught;
    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        buildingLevels[i] = in.buildingLevels[i];
    }
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        goldUpgradesBought[i] = in.goldUpgradesBought[i];
    }

    revision++;
    applyHeadStart();

    // Force update() to re-baseline its clock instead of crediting the gap since boot
    started = false;
}
