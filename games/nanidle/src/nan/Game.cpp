#include "Game.h"
#include <math.h>
#include <string.h>

Game::Game(uint32_t (*random)()) : random(random)
{
    newGame();
}

void Game::newGame()
{
    memset(skills, 0, sizeof(skills));
    memset(augments, 0, sizeof(augments));
    memset(xpBoughtCount, 0, sizeof(xpBoughtCount));
    memset(itemLevels, 0, sizeof(itemLevels));
    memset(minerCounts, 0, sizeof(minerCounts));
    for (uint8_t i = 0; i < EQUIP_COUNT; i++)
    {
        equipped[i] = -1;
    }
    xpNow = 0;
    bestBeaten = 0;
    nanPoints = 0;
    rebirthCount = 0;
    playTime = 0;
    unlocked = 0;
    unlockCount = 0;
    lastUnlocked = FEAT_BOSS;
    advKillCount = 0;
    advDeathCount = 0;
    swingCount = 0;
    lastSwingDamage = 0;
    lastSwingWasStrike = false;
    enemySwingCount = 0;
    dropCount = 0;
    lastDropItem = -1;
    bossKillCount = 0;
    milestoneCount = 0;
    clearRun();
    energyNow = energyCap() / 2;
}

void Game::clearRun()
{
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        skills[i] = Skill{0, 0, 0};
    }
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        augments[i] = Augment{0, 0, 0};
    }
    memset(minerCounts, 0, sizeof(minerCounts));
    goldNow = 0;
    beaten = 0;
    bossHpNow = bossMaxHp(0);
    fighting = false;
    runTime = 0;
    zoneNow = 0;
    enemyStateNow = EnemyState::NONE;
    enemyIndex = 0;
    enemyHpNow = 0;
    respawnTimer = 0;
    swingTimer = 0;
    enemySwingTimer = 0;
    strikeTimer = 0;
    killsSinceBoss = 0;
    bossPlayerHpNow = maxHp();
    advHpNow = maxHp();
    lastMaxHp = maxHp();
}

// ---- Save -----------------------------------------------------------------------------------

void Game::capture(Snapshot &out) const
{
    memset(&out, 0, sizeof(out));
    out.version = SNAPSHOT_VERSION;
    out.zone = zoneNow;
    out.fighting = fighting;
    memcpy(out.equipped, equipped, sizeof(equipped));
    out.unlocked = unlocked;
    out.bossesBeaten = beaten;
    out.bestBosses = bestBeaten;
    out.rebirths = rebirthCount;
    memcpy(out.xpBought, xpBoughtCount, sizeof(xpBoughtCount));
    memcpy(out.miners, minerCounts, sizeof(minerCounts));
    memcpy(out.itemLevel, itemLevels, sizeof(itemLevels));
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        out.skillLevel[i] = skills[i].level;
        out.skillProgress[i] = (float)skills[i].progress;
        out.skillAlloc[i] = (float)skills[i].alloc;
    }
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        out.augmentLevel[i] = augments[i].level;
        out.augmentProgress[i] = (float)augments[i].progress;
        out.augmentAlloc[i] = (float)augments[i].alloc;
    }
    out.energy = (float)energyNow;
    out.bossHp = (float)bossHpNow;
    out.bossPlayerHp = (float)bossPlayerHpNow;
    out.advHp = (float)advHpNow;
    out.nanPoints = nanPoints;
    out.runSeconds = (uint32_t)runTime;
    out.playSeconds = (uint32_t)playTime;
    out.xp = xpNow;
    out.gold = goldNow;
}

bool Game::restore(const Snapshot &in)
{
    newGame();
    if (in.version != SNAPSHOT_VERSION || in.zone > ZONE_COUNT)
    {
        return false;
    }
    for (uint8_t i = 0; i < EQUIP_COUNT; i++)
    {
        if (in.equipped[i] >= ITEM_COUNT || in.equipped[i] < -1)
        {
            return false;
        }
    }
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (in.itemLevel[i] > ITEM_MAX_LEVEL)
        {
            return false;
        }
    }

    memcpy(equipped, in.equipped, sizeof(equipped));
    unlocked = in.unlocked;
    beaten = in.bossesBeaten;
    bestBeaten = in.bestBosses;
    rebirthCount = in.rebirths;
    memcpy(xpBoughtCount, in.xpBought, sizeof(xpBoughtCount));
    memcpy(minerCounts, in.miners, sizeof(minerCounts));
    memcpy(itemLevels, in.itemLevel, sizeof(itemLevels));
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        skills[i] = Skill{in.skillLevel[i], in.skillProgress[i], in.skillAlloc[i]};
    }
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        augments[i] = Augment{in.augmentLevel[i], in.augmentProgress[i], in.augmentAlloc[i]};
    }
    nanPoints = in.nanPoints;
    runTime = in.runSeconds;
    playTime = in.playSeconds;
    xpNow = in.xp;
    goldNow = in.gold;
    energyNow = in.energy;
    // Float rounding could leave the allocations a hair over what the energy holds
    if (energyNow < allocated())
    {
        energyNow = allocated();
    }
    bossHpNow = in.bossHp > 0 ? in.bossHp : bossMaxHp(beaten);
    bossPlayerHpNow = in.bossPlayerHp;
    advHpNow = in.advHp;
    lastMaxHp = maxHp();
    fighting = in.fighting != 0;
    setZone(in.zone);
    return true;
}

// ---- Energy ---------------------------------------------------------------------------------

double Game::energyCap() const
{
    return ENERGY_CAP_BASE + XP_UPGRADES[(uint8_t)XpUpgrade::CAP].gain * xpBought(XpUpgrade::CAP);
}

double Game::energyRegen() const
{
    return ENERGY_REGEN_BASE + XP_UPGRADES[(uint8_t)XpUpgrade::REGEN].gain * xpBought(XpUpgrade::REGEN);
}

double Game::energyPower() const
{
    double base = ENERGY_POWER_BASE + XP_UPGRADES[(uint8_t)XpUpgrade::POWER].gain * xpBought(XpUpgrade::POWER);
    return base * (1 + augmentPercent(AugmentEffect::POWER) / 100) * (1 + rebirthPercent() / 100);
}

double Game::allocated() const
{
    double sum = 0;
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        sum += skills[i].alloc;
    }
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        sum += augments[i].alloc;
    }
    return sum;
}

double Game::allocStep() const
{
    double step = floor(energyCap() * ALLOC_STEP_SHARE);
    return step < 1 ? 1 : step;
}

double &Game::allocSlot(Target t)
{
    return t.augment ? augments[t.index].alloc : skills[t.index].alloc;
}

double Game::allocSlotValue(Target t) const
{
    return t.augment ? augments[t.index].alloc : skills[t.index].alloc;
}

double Game::allocatedTo(Target t) const
{
    return allocSlotValue(t);
}

double Game::allocate(Target t, double amount)
{
    if (!t.augment && !skillOpen(t.index))
    {
        return 0;
    }
    double idle = floor(idleEnergy());
    if (amount > idle)
    {
        amount = idle;
    }
    if (amount <= 0)
    {
        return 0;
    }
    allocSlot(t) += amount;
    return amount;
}

double Game::reclaim(Target t, double amount)
{
    double &slot = allocSlot(t);
    if (amount > slot)
    {
        amount = slot;
    }
    slot -= amount;
    return amount;
}

void Game::reclaimAll()
{
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        skills[i].alloc = 0;
    }
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        augments[i].alloc = 0;
    }
}

void Game::tickEnergy(double dt)
{
    double cap = energyCap();
    if (energyNow < cap)
    {
        energyNow += energyRegen() * dt;
        if (energyNow > cap)
        {
            energyNow = cap;
        }
    }
}

// ---- Training -------------------------------------------------------------------------------

bool Game::skillOpen(uint8_t i) const
{
    const SkillDef &def = SKILLS[i];
    return def.parent < 0 || skills[def.parent].level >= def.parentLevel;
}

double Game::skillProgress(uint8_t i) const
{
    return skills[i].progress / SKILLS[i].levelCost;
}

double Game::skillSecondsPerLevel(uint8_t i) const
{
    double rate = skills[i].alloc * energyPower();
    return rate > 0 ? SKILLS[i].levelCost / rate : 0;
}

double Game::skillStat(uint8_t i) const
{
    return SKILLS[i].statPerLevel * pow((double)skills[i].level, SKILL_EXPONENT);
}

void Game::tickTraining(double dt)
{
    double power = energyPower();
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        Skill &skill = skills[i];
        if (skill.alloc <= 0)
        {
            continue;
        }
        skill.progress += skill.alloc * power * dt;
        double cost = SKILLS[i].levelCost;
        if (skill.progress >= cost)
        {
            double levels = floor(skill.progress / cost);
            skill.level += (uint32_t)levels;
            skill.progress -= levels * cost;
        }
    }
}

// ---- Stats ----------------------------------------------------------------------------------

double Game::itemPercent(uint8_t item, bool attackStat) const
{
    uint8_t level = itemLevels[item];
    if (level == 0)
    {
        return 0;
    }
    double base = attackStat ? ITEMS[item].attack : ITEMS[item].defense;
    return base * (1 + ITEM_GROWTH_PER_LEVEL * (level - 1));
}

double Game::gearPercent(bool attackStat) const
{
    double sum = 0;
    for (uint8_t place = 0; place < EQUIP_COUNT; place++)
    {
        if (equipped[place] >= 0)
        {
            sum += itemPercent(equipped[place], attackStat);
        }
    }
    return sum;
}

double Game::augmentPercent(AugmentEffect effect) const
{
    double sum = 0;
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        if (AUGMENTS[i].effect == effect)
        {
            sum += AUGMENTS[i].percentPerLevel * augments[i].level;
        }
    }
    return sum;
}

double Game::attack() const
{
    double raw = BASE_ATTACK;
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        if (SKILLS[i].kind == SkillKind::ATTACK)
        {
            raw += skillStat(i);
        }
    }
    return raw * (1 + gearPercent(true) / 100) * (1 + augmentPercent(AugmentEffect::ATTACK) / 100) *
           (1 + rebirthPercent() / 100);
}

double Game::defense() const
{
    double raw = BASE_DEFENSE;
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        if (SKILLS[i].kind == SkillKind::DEFENSE)
        {
            raw += skillStat(i);
        }
    }
    return raw * (1 + gearPercent(false) / 100) * (1 + augmentPercent(AugmentEffect::DEFENSE) / 100) *
           (1 + rebirthPercent() / 100);
}

// ---- Boss ladder ----------------------------------------------------------------------------

double Game::bossAttack(uint16_t index)
{
    return BOSS_ATTACK_0 * pow(BOSS_GROWTH, index);
}

double Game::bossDefense(uint16_t index)
{
    return bossAttack(index) * BOSS_DEFENSE_RATIO;
}

double Game::bossMaxHp(uint16_t index)
{
    return bossAttack(index) * BOSS_HP_RATIO;
}

double Game::bossXp(uint16_t index)
{
    return floor(BOSS_XP_0 * pow(BOSS_XP_GROWTH, index));
}

// The web game's rule, with no floor: a defense at or over the attack blocks it whole
double Game::bossDps() const
{
    double d = attack() - bossDefense(beaten);
    return d > 0 ? d : 0;
}

double Game::bossHurts() const
{
    double d = bossAttack(beaten) - defense();
    return d > 0 ? d : 0;
}

bool Game::setFighting(bool on)
{
    if (on && bossPlayerHpNow < maxHp() / 10)
    {
        return false;
    }
    fighting = on;
    return true;
}

void Game::tickBoss(double dt)
{
    double hpMax = maxHp();
    bossPlayerHpNow += hpRegen() * dt;
    if (bossPlayerHpNow > hpMax)
    {
        bossPlayerHpNow = hpMax;
    }

    double bossMax = bossMaxHp(beaten);
    if (fighting)
    {
        bossPlayerHpNow -= bossHurts() * dt;
        if (bossPlayerHpNow <= 0)
        {
            bossPlayerHpNow = 0;
            fighting = false;
        }
        else
        {
            bossHpNow -= bossDps() * dt;
            if (bossHpNow <= 0)
            {
                xpNow += bossXp(beaten);
                beaten++;
                if (beaten > bestBeaten)
                {
                    bestBeaten = beaten;
                }
                bossKillCount++;
                milestoneCount++;
                bossHpNow = bossMaxHp(beaten);
                return;
            }
        }
    }
    bossHpNow += bossMax * BOSS_REGEN_SHARE * dt;
    if (bossHpNow > bossMax)
    {
        bossHpNow = bossMax;
    }
}

// ---- Spend XP -------------------------------------------------------------------------------

double Game::xpCost(XpUpgrade u) const
{
    const XpUpgradeDef &def = XP_UPGRADES[(uint8_t)u];
    return ceil(def.baseCost * pow(def.costGrowth, xpBought(u)));
}

bool Game::buyXp(XpUpgrade u)
{
    double cost = xpCost(u);
    if (xpNow < cost || xpBoughtCount[(uint8_t)u] == UINT16_MAX)
    {
        return false;
    }
    xpNow -= cost;
    xpBoughtCount[(uint8_t)u]++;
    milestoneCount++;
    return true;
}

double Game::luck() const
{
    return 1 + XP_UPGRADES[(uint8_t)XpUpgrade::LUCK].gain * xpBought(XpUpgrade::LUCK);
}

// ---- Adventure ------------------------------------------------------------------------------

bool Game::zoneOpen(uint8_t z) const
{
    return z == 0 || (z <= ZONE_COUNT && beaten >= ZONES[z - 1].reqBosses && isUnlocked(FEAT_ADVENTURE));
}

void Game::setZone(uint8_t z)
{
    if (!zoneOpen(z))
    {
        z = 0;
    }
    zoneNow = z;
    killsSinceBoss = 0;
    strikeTimer = 0;
    if (z == 0)
    {
        enemyStateNow = EnemyState::NONE;
        return;
    }
    enemyStateNow = EnemyState::RESPAWN;
    respawnTimer = ADV_RESPAWN_S;
}

const EnemyDef *Game::enemy() const
{
    const ZoneDef *def = zoneDef();
    if (def == nullptr || enemyStateNow != EnemyState::FIGHTING)
    {
        return nullptr;
    }
    return enemyIsBoss() ? &def->boss : &def->enemies[enemyIndex];
}

double Game::enemyAttack() const
{
    const EnemyDef *e = enemy();
    return e == nullptr ? 0 : bossAttack(zoneDef()->refBoss) * e->attack;
}

double Game::enemyDefense() const
{
    const EnemyDef *e = enemy();
    return e == nullptr ? 0 : bossDefense(zoneDef()->refBoss) * e->defense;
}

double Game::enemyMaxHp() const
{
    const EnemyDef *e = enemy();
    return e == nullptr ? 0 : bossMaxHp(zoneDef()->refBoss) * e->hp;
}

// The web adventure's rule: defense blocks half its value of each hit
double Game::playerHit() const
{
    double d = attack() - enemyDefense() / 2;
    return d > 0 ? d : 0;
}

double Game::enemyHit() const
{
    double d = enemyAttack() - defense() / 2;
    return d > 0 ? d : 0;
}

double Game::strikeReady() const
{
    return 1 - strikeTimer / ADV_STRIKE_COOLDOWN_S;
}

bool Game::strike()
{
    if (enemyStateNow != EnemyState::FIGHTING || strikeTimer > 0)
    {
        return false;
    }
    double damage = attack() * ADV_STRIKE_MULTIPLIER - enemyDefense() / 2;
    if (damage <= 0)
    {
        return false;
    }
    strikeTimer = ADV_STRIKE_COOLDOWN_S;
    hitEnemy(damage, true);
    return true;
}

void Game::spawnEnemy()
{
    if (killsSinceBoss >= ZONE_BOSS_EVERY)
    {
        enemyIndex = ZONE_ENEMIES;
        killsSinceBoss = 0;
    }
    else
    {
        enemyIndex = random() % ZONE_ENEMIES;
    }
    enemyStateNow = EnemyState::FIGHTING;
    enemyHpNow = enemyMaxHp();
    swingTimer = ADV_PLAYER_SWING_S;
    enemySwingTimer = ADV_ENEMY_SWING_S;
}

void Game::hitEnemy(double damage, bool strike)
{
    swingCount++;
    lastSwingDamage = damage;
    lastSwingWasStrike = strike;
    enemyHpNow -= damage;
    if (enemyHpNow <= 0)
    {
        killEnemy();
    }
}

void Game::killEnemy()
{
    const ZoneDef &def = *zoneDef();
    bool boss = enemyIsBoss();
    advKillCount++;
    if (!boss)
    {
        killsSinceBoss++;
    }

    addGold(def.gold * (boss ? BOSS_GOLD_FACTOR : 1));
    if (roll() < XP_DROP_CHANCE * (boss ? BOSS_DROP_BONUS : 1))
    {
        xpNow += def.xp;
    }
    double chanceScale = luck() * (boss ? BOSS_DROP_BONUS : 1);
    for (uint8_t i = 0; i < ZONE_DROPS; i++)
    {
        const DropDef &drop = def.drops[i];
        if (drop.item < 0)
        {
            continue;
        }
        if (roll() < drop.chance * chanceScale)
        {
            giveItem(drop.item);
            dropCount++;
            lastDropItem = drop.item;
            break;
        }
    }

    enemyStateNow = EnemyState::RESPAWN;
    respawnTimer = ADV_RESPAWN_S;
}

void Game::tickAdventure(double dt)
{
    double hpMax = maxHp();
    advHpNow += hpRegen() * dt * (zoneNow == 0 ? 5 : 1);
    if (advHpNow > hpMax)
    {
        advHpNow = hpMax;
    }
    if (strikeTimer > 0)
    {
        strikeTimer -= dt;
        if (strikeTimer < 0)
        {
            strikeTimer = 0;
        }
    }

    if (enemyStateNow == EnemyState::RESPAWN)
    {
        respawnTimer -= dt;
        if (respawnTimer <= 0)
        {
            spawnEnemy();
        }
        return;
    }
    if (enemyStateNow != EnemyState::FIGHTING)
    {
        return;
    }

    swingTimer -= dt;
    if (swingTimer <= 0)
    {
        swingTimer += ADV_PLAYER_SWING_S;
        double damage = playerHit();
        if (damage > 0)
        {
            hitEnemy(damage, false);
            if (enemyStateNow != EnemyState::FIGHTING)
            {
                return;
            }
        }
    }

    enemySwingTimer -= dt;
    if (enemySwingTimer <= 0)
    {
        enemySwingTimer += ADV_ENEMY_SWING_S;
        enemySwingCount++;
        advHpNow -= enemyHit();
        if (advHpNow <= 0)
        {
            advHpNow = 0;
            advDeathCount++;
            setZone(0);
        }
    }
}

// ---- Items ----------------------------------------------------------------------------------

bool Game::equip(uint8_t place, int8_t item)
{
    if (place >= EQUIP_COUNT)
    {
        return false;
    }
    if (item < 0)
    {
        equipped[place] = -1;
        return true;
    }
    if (item >= ITEM_COUNT || itemLevels[item] == 0 || ITEMS[item].slot != placeSlot(place))
    {
        return false;
    }
    for (uint8_t other = 0; other < EQUIP_COUNT; other++)
    {
        if (equipped[other] == item)
        {
            equipped[other] = -1;
        }
    }
    equipped[place] = item;
    milestoneCount++;
    return true;
}

void Game::giveItem(uint8_t item)
{
    if (itemLevels[item] < ITEM_MAX_LEVEL)
    {
        itemLevels[item]++;
    }
    milestoneCount++;
}

// ---- Gold -----------------------------------------------------------------------------------

void Game::addGold(double amount)
{
    goldNow += amount * (1 + rebirthPercent() / 100);
}

double Game::goldPerSecond() const
{
    double sum = 0;
    for (uint8_t i = 0; i < MINER_COUNT; i++)
    {
        sum += MINERS[i].goldPerSecond * minerCounts[i];
    }
    return sum * (1 + rebirthPercent() / 100);
}

double Game::minerCost(uint8_t i) const
{
    return ceil(MINERS[i].cost * pow(MINER_COST_GROWTH, minerCounts[i]));
}

bool Game::buyMiner(uint8_t i)
{
    double cost = minerCost(i);
    if (goldNow < cost || minerCounts[i] == UINT16_MAX)
    {
        return false;
    }
    goldNow -= cost;
    minerCounts[i]++;
    milestoneCount++;
    return true;
}

double Game::augmentProgress(uint8_t i) const
{
    return augments[i].progress / AUGMENTS[i].levelCost;
}

double Game::augmentGoldCost(uint8_t i) const
{
    return ceil(AUGMENTS[i].goldCost * pow(AUGMENT_GOLD_GROWTH, augments[i].level));
}

bool Game::augmentStalled(uint8_t i) const
{
    return augments[i].progress >= AUGMENTS[i].levelCost && goldNow < augmentGoldCost(i);
}

void Game::tickAugments(double dt)
{
    double power = energyPower();
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        Augment &aug = augments[i];
        double cost = AUGMENTS[i].levelCost;
        if (aug.alloc > 0 && aug.progress < cost)
        {
            aug.progress += aug.alloc * power * dt;
        }
        // A level is paid when its bar fills; without the gold the bar waits full
        while (aug.progress >= cost && aug.level < UINT16_MAX)
        {
            double price = augmentGoldCost(i);
            if (goldNow < price)
            {
                aug.progress = cost;
                break;
            }
            goldNow -= price;
            aug.level++;
            aug.progress -= cost;
        }
    }
}

// ---- Rebirth --------------------------------------------------------------------------------

uint32_t Game::rebirthPoints() const
{
    if (!canRebirth())
    {
        return 0;
    }
    return (uint32_t)floor(pow((double)(beaten - REBIRTH_MIN_BOSSES + 1), REBIRTH_EXPONENT));
}

void Game::rebirth()
{
    if (!canRebirth())
    {
        return;
    }
    nanPoints += rebirthPoints();
    rebirthCount++;
    clearRun();
    energyNow = energyCap();
    milestoneCount++;
}

// ---- Unlocks --------------------------------------------------------------------------------

void Game::unlock(Feature f)
{
    if (isUnlocked(f))
    {
        return;
    }
    unlocked |= 1u << f;
    unlockCount++;
    lastUnlocked = f;
    milestoneCount++;
}

void Game::checkUnlocks()
{
    if (attack() >= BOSS_UNLOCK_ATTACK)
    {
        unlock(FEAT_BOSS);
    }
    if (beaten >= XP_UNLOCK_BOSSES)
    {
        unlock(FEAT_XP);
    }
    if (beaten >= ZONES[0].reqBosses)
    {
        unlock(FEAT_ADVENTURE);
    }
    if (goldNow > 0)
    {
        unlock(FEAT_MINERS);
    }
    if (beaten >= AUGMENT_UNLOCK_BOSSES)
    {
        unlock(FEAT_AUGMENTS);
    }
    if (beaten >= REBIRTH_MIN_BOSSES)
    {
        unlock(FEAT_REBIRTH);
    }
}

// ---- Tick -----------------------------------------------------------------------------------

void Game::tick(double dt)
{
    playTime += dt;
    runTime += dt;
    tickEnergy(dt);
    tickTraining(dt);
    tickAugments(dt);
    double hpMax = maxHp();
    if (hpMax > lastMaxHp)
    {
        bossPlayerHpNow += hpMax - lastMaxHp;
        advHpNow += hpMax - lastMaxHp;
    }
    lastMaxHp = hpMax;
    goldNow += goldPerSecond() * dt;
    tickBoss(dt);
    tickAdventure(dt);
    checkUnlocks();
}
