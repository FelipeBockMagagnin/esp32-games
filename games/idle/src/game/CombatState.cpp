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
      spellReadyAt(0),
      wardEndsAt(0),
      midasArmed(false),
      guardReadyAt(0),
      guardEndsAt(0),
      respawnAt(0),
      reviveAt(0),
      deathAt(0),
      regenAt(0),
      regenCarry(0),
      killEvents(0),
      lastKillGold(0),
      lastKillMidas(false),
      hitEvents(0),
      lastHitDamage(0),
      lastHitKind(HIT_AUTO),
      hurtEvents(0),
      lastHurtDamage(0),
      lastHurtGuarded(false),
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
    startRespawn(millis());
    return true;
}

void CombatState::leaveZone()
{
    inZone = false;
    phase = IDLE;
}

void CombatState::rerollEnemy(unsigned long now)
{
    if (!inZone || phase != FIGHTING)
    {
        return;
    }
    pickEnemy(currentEnemy);
    startFight(now);
}

void CombatState::pickEnemy(uint8_t avoid)
{
    const ZoneDef &zone = ZONES[currentZone];
    if (zone.enemyCount < 2)
    {
        currentEnemy = 0;
    }
    else if (avoid < zone.enemyCount)
    {
        // One draw from the others, shifted past the one to avoid, keeps them equally likely
        currentEnemy = (uint8_t)random(zone.enemyCount - 1);
        if (currentEnemy >= avoid)
        {
            currentEnemy++;
        }
    }
    else
    {
        currentEnemy = (uint8_t)random(zone.enemyCount);
    }
    enemyHp = zone.enemies[currentEnemy].maxHp;
}

void CombatState::startFight(unsigned long now)
{
    phase = FIGHTING;
    autoAttackAt = now + AUTO_ATTACK_MS;
    enemyAttackAt = now + getEnemy().attackIntervalMs;
}

void CombatState::startRespawn(unsigned long now)
{
    phase = RESPAWNING;
    respawnAt = now + RESPAWN_MS * (100 - inventory.getRespawnCutPercent()) / 100;
}

uint32_t CombatState::damageTo(uint32_t defense, uint8_t multiplier) const
{
    return damageThrough(cachedStats.attack * multiplier, defense);
}

uint32_t CombatState::getAutoAttackDamage() const
{
    return phase == IDLE ? 0 : damageTo(getEnemy().defense, 1);
}

uint32_t CombatState::getStrikeDamage() const
{
    return phase == IDLE ? 0 : damageTo(getEnemy().defense, STRIKE_MULTIPLIER);
}

uint32_t CombatState::getBestAttackDamage() const
{
    // Spells deal no damage, so the strike is the hardest hit there is
    return getStrikeDamage();
}

const SpellDef *CombatState::getSpell() const
{
    return spellForItem(inventory.getEquipped(EquipSlot::AMULET));
}

bool CombatState::isSpellUseless() const
{
    const SpellDef *spell = getSpell();
    if (spell == nullptr)
    {
        return true;
    }
    switch (spell->effect)
    {
    case SpellEffect::HEAL:
        return playerHp >= cachedStats.maxHp;
    case SpellEffect::MIDAS:
        return midasArmed;
    default:
        return false;
    }
}

bool CombatState::isWarded(unsigned long now) const
{
    return wardEndsAt != 0 && !reached(now, wardEndsAt);
}

unsigned long CombatState::getWardRemaining(unsigned long now) const
{
    return isWarded(now) ? wardEndsAt - now : 0;
}

uint32_t CombatState::getEnemyAttackDamage() const
{
    if (phase == IDLE)
    {
        return 0;
    }
    return damageThrough(getEnemy().attack, cachedStats.defense);
}

uint32_t CombatState::getEnemyHitDamage(unsigned long now) const
{
    uint32_t damage = getEnemyAttackDamage();
    return isGuarding(now) ? damage * GUARD_DAMAGE_PERCENT / 100 : damage;
}

// Every hit on the enemy goes through here, so the screen hears about each one
void CombatState::hitEnemy(uint32_t damage, HitKind kind, unsigned long now)
{
    lastHitDamage = damage;
    lastHitKind = kind;
    hitEvents++;

    enemyHp = enemyHp > damage ? enemyHp - damage : 0;
    if (enemyHp == 0)
    {
        onEnemyKilled(now);
    }
}

void CombatState::onEnemyKilled(unsigned long now)
{
    const EnemyDef &enemy = getEnemy();

    // An armed Midas multiplies this kill's gold and is spent by it
    lastKillMidas = midasArmed;
    lastKillGold = enemy.goldReward;
    if (midasArmed)
    {
        const SpellDef *spell = getSpell();
        lastKillGold *= spell != nullptr && spell->effect == SpellEffect::MIDAS ? spell->power : 1;
        midasArmed = false;
    }
    game.addGold(lastKillGold);
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

    startRespawn(now);
}

void CombatState::regenerate(unsigned long now)
{
    if (!reached(now, regenAt))
    {
        return;
    }
    regenAt = now + REGEN_TICK_MS;

    uint32_t tenths = regenCarry + cachedStats.regen;
    uint32_t gained = tenths / 10;
    regenCarry = (uint8_t)(tenths % 10);
    playerHp = playerHp + gained < cachedStats.maxHp ? playerHp + gained : cachedStats.maxHp;
}

void CombatState::onPlayerDied(unsigned long now)
{
    playerHp = 0;
    phase = DEAD;
    deathAt = now;
    reviveAt = now + REVIVE_MS;
    guardEndsAt = 0; // Losing the fight drops the guard and the ward
    wardEndsAt = 0;
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
        regenerate(now);
        if (reached(now, respawnAt))
        {
            pickEnemy();
            startFight(now);
        }
        return;

    case FIGHTING:
        regenerate(now);
        break;

    default:
        return;
    }

    // Clearing it here keeps isGuarding() from misreading a stale deadline across a rollover
    if (guardEndsAt != 0 && reached(now, guardEndsAt))
    {
        guardEndsAt = 0;
    }
    if (wardEndsAt != 0 && reached(now, wardEndsAt))
    {
        wardEndsAt = 0;
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
            hitEnemy(damage, HIT_AUTO, now);
            if (phase != FIGHTING)
            {
                return;
            }
        }
    }

    if (reached(now, enemyAttackAt))
    {
        enemyAttackAt = now + enemy.attackIntervalMs;

        uint32_t damage = getEnemyHitDamage(now);
        bool guarded = isGuarding(now);
        if (isWarded(now))
        {
            damage = 0;
        }

        if (damage > 0)
        {
            lastHurtDamage = damage;
            lastHurtGuarded = guarded;
            hurtEvents++;
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
    hitEnemy(damage, HIT_STRIKE, now);
    return true;
}

bool CombatState::castSpell()
{
    unsigned long now = millis();
    const SpellDef *spell = getSpell();
    if (phase != FIGHTING || spell == nullptr || !reached(now, spellReadyAt) || isSpellUseless())
    {
        return false;
    }

    switch (spell->effect)
    {
    case SpellEffect::HEAL:
    {
        uint32_t heal = cachedStats.maxHp * spell->power / 100;
        playerHp = playerHp + heal < cachedStats.maxHp ? playerHp + heal : cachedStats.maxHp;
        break;
    }
    case SpellEffect::STUN:
        enemyAttackAt += getEnemy().attackIntervalMs * spell->power;
        break;
    case SpellEffect::MIDAS:
        midasArmed = true;
        break;
    case SpellEffect::WARD:
        wardEndsAt = now + spell->power;
        break;
    }

    spellReadyAt = now + spell->cooldownMs;
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

unsigned long CombatState::getSpellCooldownRemaining(unsigned long now) const
{
    return remainingUntil(now, spellReadyAt);
}

unsigned long CombatState::getGuardCooldownRemaining(unsigned long now) const
{
    return remainingUntil(now, guardReadyAt);
}

unsigned long CombatState::getReviveRemaining(unsigned long now) const
{
    return phase == DEAD ? remainingUntil(now, reviveAt) : 0;
}

unsigned long CombatState::getRespawnRemaining(unsigned long now) const
{
    return phase == RESPAWNING ? remainingUntil(now, respawnAt) : 0;
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
