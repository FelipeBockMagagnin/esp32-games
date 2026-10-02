#include "CombatState.h"
#include "GameState.h"
#include "Inventory.h"

// Every deadline is compared with a signed difference so the fight survives a
// millis() rollover, the same way GameState::update does
static bool reached(unsigned long now, unsigned long deadline)
{
    return (long)(now - deadline) >= 0;
}

static unsigned long remainingUntil(unsigned long now, unsigned long deadline)
{
    return reached(now, deadline) ? 0 : deadline - now;
}

CombatState::CombatState(GameState &game, Inventory &inventory)
    : game(game),
      inventory(inventory),
      currentZone(0),
      inZone(false),
      currentEnemy(0),
      enemyHp(0),
      playerHp(0),
      phase(IDLE),
      cachedStats({BASE_ATTACK, BASE_DEFENSE, BASE_MAX_HP}),
      seenInventoryRevision(0),
      autoAttackAt(0),
      enemyAttackAt(0),
      strikeReadyAt(0),
      smiteReadyAt(0),
      guardReadyAt(0),
      guardEndsAt(0),
      respawnAt(0),
      reviveAt(0),
      deathAt(0),
      killEvents(0),
      dropEvents(0),
      lastDrop(NO_ITEM),
      lastDropWasNew(false),
      lastDropLevel(0)
{
    for (uint8_t i = 0; i < ZONE_COUNT; i++)
    {
        zoneKills[i] = 0;
    }
}

void CombatState::refreshStats()
{
    uint32_t revision = inventory.getRevision();
    if (revision == seenInventoryRevision && cachedStats.maxHp > 0)
    {
        return;
    }
    seenInventoryRevision = revision;

    uint32_t previousMax = cachedStats.maxHp;
    cachedStats = resolvePlayerStats(game, inventory);

    // Gear that raises max health grants the difference right away rather than
    // leaving the bar looking half empty
    if (cachedStats.maxHp > previousMax)
    {
        playerHp += cachedStats.maxHp - previousMax;
    }
    if (playerHp > cachedStats.maxHp)
    {
        playerHp = cachedStats.maxHp;
    }
}

bool CombatState::isZoneUnlocked(uint8_t zoneId) const
{
    if (zoneId >= ZONE_COUNT)
    {
        return false;
    }
    if (zoneId == 0)
    {
        return true;
    }

    const ZoneDef &def = ZONES[zoneId];
    if (def.requiresUnlockUpgrade && !game.isZoneUnlockedByUpgrade(zoneId))
    {
        return false;
    }
    return zoneKills[zoneId - 1] >= def.reqPrevZoneKills;
}

bool CombatState::enterZone(uint8_t zoneId)
{
    if (!isZoneUnlocked(zoneId))
    {
        return false;
    }

    currentZone = zoneId;
    inZone = true;
    refreshStats();
    playerHp = cachedStats.maxHp;
    pickEnemy();
    startFight(millis());
    return true;
}

void CombatState::leaveZone()
{
    inZone = false;
    phase = IDLE;
}

void CombatState::pickEnemy()
{
    const ZoneDef &zone = ZONES[currentZone];
    currentEnemy = zone.enemyCount > 1 ? (uint8_t)random(zone.enemyCount) : 0;
    enemyHp = zone.enemies[currentEnemy].maxHp;
}

void CombatState::startFight(unsigned long now)
{
    phase = FIGHTING;
    autoAttackAt = now + AUTO_ATTACK_MS;
    enemyAttackAt = now + getEnemy().attackIntervalMs;
}

uint32_t CombatState::damageTo(uint32_t defense, uint8_t multiplier) const
{
    uint32_t attack = cachedStats.attack * multiplier;
    // No floor: an attack that cannot beat the defense does nothing at all. That is what
    // stops the auto-attack from grinding down anything given enough time, and makes
    // better gear the only way into a tougher zone.
    return attack > defense ? attack - defense : 0;
}

uint32_t CombatState::getAutoAttackDamage() const
{
    return phase == IDLE ? 0 : damageTo(getEnemy().defense, 1);
}

uint32_t CombatState::getStrikeDamage() const
{
    return phase == IDLE ? 0 : damageTo(getEnemy().defense, STRIKE_MULTIPLIER);
}

uint32_t CombatState::getSmiteDamage() const
{
    if (phase == IDLE || !isSmiteUnlocked())
    {
        return 0;
    }
    return damageTo(getEnemy().defense, SMITE_MULTIPLIER);
}

uint32_t CombatState::getBestAttackDamage() const
{
    uint32_t strikeDamage = getStrikeDamage();
    uint32_t smiteDamage = getSmiteDamage();
    return smiteDamage > strikeDamage ? smiteDamage : strikeDamage;
}

bool CombatState::isSmiteUnlocked() const
{
    return inventory.getEquipped(EquipSlot::AMULET) != NO_ITEM;
}

uint32_t CombatState::getEnemyAttackDamage() const
{
    if (phase == IDLE)
    {
        return 0;
    }
    uint32_t attack = getEnemy().attack;
    return attack > cachedStats.defense ? attack - cachedStats.defense : 0;
}

void CombatState::onEnemyKilled(unsigned long now)
{
    const EnemyDef &enemy = getEnemy();

    game.addGold(enemy.goldReward);
    if (zoneKills[currentZone] < UINT16_MAX)
    {
        zoneKills[currentZone]++;
    }
    killEvents++;

    for (uint8_t i = 0; i < enemy.dropCount; i++)
    {
        const DropDef &drop = enemy.drops[i];
        if ((uint16_t)random(10000) < drop.chanceBp)
        {
            lastDropWasNew = inventory.addDrop(drop.itemId);
            lastDrop = (int8_t)drop.itemId;
            lastDropLevel = inventory.getItemLevel(drop.itemId);
            dropEvents++;
        }
    }

    phase = RESPAWNING;
    respawnAt = now + RESPAWN_MS;
}

void CombatState::onPlayerDied(unsigned long now)
{
    playerHp = 0;
    phase = DEAD;
    deathAt = now;
    reviveAt = now + REVIVE_MS;
    guardEndsAt = 0; // Losing the fight drops the guard
}

void CombatState::update(unsigned long now)
{
    if (!inZone)
    {
        return;
    }

    refreshStats();

    switch (phase)
    {
    case DEAD:
        // Fill the bar as the timer runs down so the wait reads as healing
        if (reached(now, reviveAt))
        {
            playerHp = cachedStats.maxHp;
            pickEnemy();
            startFight(now);
        }
        else
        {
            unsigned long elapsed = now - deathAt;
            playerHp = (uint32_t)((uint64_t)cachedStats.maxHp * elapsed / REVIVE_MS);
        }
        return;

    case RESPAWNING:
        if (reached(now, respawnAt))
        {
            pickEnemy();
            startFight(now);
        }
        return;

    case FIGHTING:
        break;

    default:
        return;
    }

    // Clearing it here keeps isGuarding() from misreading a stale deadline across a rollover
    if (guardEndsAt != 0 && reached(now, guardEndsAt))
    {
        guardEndsAt = 0;
    }

    const EnemyDef &enemy = getEnemy();

    if (reached(now, autoAttackAt))
    {
        // Deadlines are pushed forward from now instead of accumulated, so a long
        // frame never produces a burst of catch-up hits
        autoAttackAt = now + AUTO_ATTACK_MS;

        uint32_t damage = damageTo(enemy.defense, 1);
        if (damage > 0)
        {
            enemyHp = enemyHp > damage ? enemyHp - damage : 0;
            if (enemyHp == 0)
            {
                onEnemyKilled(now);
                return;
            }
        }
    }

    if (reached(now, enemyAttackAt))
    {
        enemyAttackAt = now + enemy.attackIntervalMs;

        uint32_t damage = enemy.attack > cachedStats.defense ? enemy.attack - cachedStats.defense : 0;
        if (isGuarding(now))
        {
            damage = damage * GUARD_DAMAGE_PERCENT / 100;
        }

        if (damage > 0)
        {
            if (playerHp > damage)
            {
                playerHp -= damage;
            }
            else
            {
                onPlayerDied(now);
            }
        }
    }
}

bool CombatState::strike()
{
    unsigned long now = millis();
    if (phase != FIGHTING || !reached(now, strikeReadyAt))
    {
        return false;
    }
    uint32_t damage = damageTo(getEnemy().defense, STRIKE_MULTIPLIER);
    if (damage == 0)
    {
        return false;
    }
    strikeReadyAt = now + STRIKE_COOLDOWN_MS;

    enemyHp = enemyHp > damage ? enemyHp - damage : 0;
    if (enemyHp == 0)
    {
        onEnemyKilled(now);
    }
    return true;
}

bool CombatState::smite()
{
    unsigned long now = millis();
    if (phase != FIGHTING || !isSmiteUnlocked() || !reached(now, smiteReadyAt))
    {
        return false;
    }

    uint32_t damage = damageTo(getEnemy().defense, SMITE_MULTIPLIER);
    if (damage == 0)
    {
        return false;
    }
    smiteReadyAt = now + SMITE_COOLDOWN_MS;

    enemyHp = enemyHp > damage ? enemyHp - damage : 0;
    if (enemyHp == 0)
    {
        onEnemyKilled(now);
    }
    return true;
}

bool CombatState::guard()
{
    unsigned long now = millis();
    if (phase != FIGHTING || !reached(now, guardReadyAt))
    {
        return false;
    }
    guardReadyAt = now + GUARD_COOLDOWN_MS;
    guardEndsAt = now + GUARD_DURATION_MS;
    return true;
}

uint16_t CombatState::getZoneKills(uint8_t zoneId) const
{
    return zoneId < ZONE_COUNT ? zoneKills[zoneId] : 0;
}

uint32_t CombatState::getTotalKills() const
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < ZONE_COUNT; i++)
    {
        total += zoneKills[i];
    }
    return total;
}

unsigned long CombatState::getAutoAttackRemaining(unsigned long now) const
{
    return phase == FIGHTING ? remainingUntil(now, autoAttackAt) : 0;
}

unsigned long CombatState::getEnemyAttackRemaining(unsigned long now) const
{
    return phase == FIGHTING ? remainingUntil(now, enemyAttackAt) : 0;
}

unsigned long CombatState::getStrikeCooldownRemaining(unsigned long now) const
{
    return remainingUntil(now, strikeReadyAt);
}

unsigned long CombatState::getSmiteCooldownRemaining(unsigned long now) const
{
    return remainingUntil(now, smiteReadyAt);
}

unsigned long CombatState::getGuardCooldownRemaining(unsigned long now) const
{
    return remainingUntil(now, guardReadyAt);
}

unsigned long CombatState::getReviveRemaining(unsigned long now) const
{
    return phase == DEAD ? remainingUntil(now, reviveAt) : 0;
}

bool CombatState::isGuarding(unsigned long now) const
{
    return guardEndsAt != 0 && !reached(now, guardEndsAt);
}

unsigned long CombatState::getGuardRemaining(unsigned long now) const
{
    return isGuarding(now) ? guardEndsAt - now : 0;
}

void CombatState::save(Snapshot &out) const
{
    out.currentZone = currentZone;
    out.inZone = inZone;
    for (uint8_t i = 0; i < ZONE_COUNT; i++)
    {
        out.zoneKills[i] = zoneKills[i];
    }
}

void CombatState::load(const Snapshot &in)
{
    currentZone = in.currentZone < ZONE_COUNT ? in.currentZone : 0;
    for (uint8_t i = 0; i < ZONE_COUNT; i++)
    {
        zoneKills[i] = in.zoneKills[i];
    }

    // A fight is never resumed mid-swing; re-entering the zone starts a fresh one
    inZone = false;
    phase = IDLE;
    if (in.inZone)
    {
        enterZone(currentZone);
    }
}
