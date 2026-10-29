#include "Run.h"
#include <math.h>
#include <string.h>

static const float TWO_PI = 6.2831853f;
static const float BACK_SPEED = 0.65f; // Of the forward speed
static const float ENEMY_RADIUS = 0.25f;
static const float ENEMY_BODY = 0.38f; // What a blow or a spell has to reach
static const float ENEMY_GAP = 0.55f;  // Closest an enemy walks up to the player
static const float PICK_REACH = 0.45f;
static const float LEAVE_REACH = 0.7f;
static const float SPELL_START = 0.3f;
static const float BLAST_RADIUS = 1.4f;
static const uint16_t HURT_STAGGER_MS = 180;
static const uint16_t CAST_ANIM_MS = 300;
static const uint16_t SEALED_NOTICE_MS = 1500;
static const uint16_t PROJECTILE_LIFE_MS = 3000;
static const float PUSH = 0.3f;
static const uint8_t SPAWN_CLEARANCE = 5; // Tiles between the start and any enemy

static uint32_t floorSeed(uint32_t seed, uint8_t floor)
{
    uint32_t h = seed ^ (floor * 0x9E3779B9u);
    h ^= h >> 16;
    h *= 0x85EBCA6Bu;
    h ^= h >> 13;
    return h;
}

static uint16_t tick(uint16_t timer, uint16_t dt)
{
    return timer > dt ? timer - dt : 0;
}

static float wrapAngle(float a)
{
    while (a < -3.14159265f)
    {
        a += TWO_PI;
    }
    while (a > 3.14159265f)
    {
        a -= TWO_PI;
    }
    return a;
}

void Run::newRun(uint32_t newSeed)
{
    seed = newSeed;
    floor = 1;
    kills = 0;
    inventory.clear();
    inventory.equipped[SLOT_WEAPON] = starterWeapon();
    inventory.hpPotions = 1;
    hp = (int16_t)maxHp();
    startFloor();
}

void Run::restore(const Snapshot &snapshot)
{
    seed = snapshot.seed;
    floor = snapshot.floor;
    hp = (int16_t)snapshot.hp;
    kills = snapshot.kills;
    inventory = snapshot.inventory;
    clampVitals();
    if (hp <= 0)
    {
        hp = 1;
    }
    startFloor();
}

void Run::save(Snapshot &out) const
{
    memset(&out, 0, sizeof(out));
    if (isDead())
    {
        return;
    }
    out.seed = seed;
    out.floor = floor;
    out.hp = floorHp;
    out.kills = floorKills;
    out.inventory = floorInventory;
}

void Run::nextFloor()
{
    if (floor < 255)
    {
        floor++;
    }
    startFloor();
}

void Run::startFloor()
{
    Rng gen(floorSeed(seed, floor));
    map.generate(gen, floor);
    memset(enemies, 0, sizeof(enemies));
    memset(pickups, 0, sizeof(pickups));
    memset(projectiles, 0, sizeof(projectiles));
    memset(sparks, 0, sizeof(sparks));
    for (Enemy &e : enemies)
    {
        e.state = EnemyState::DEAD;
    }

    const Room &start = map.rooms[0];
    px = start.centerX() + 0.5f;
    py = start.centerY() + 0.5f;
    if (map.roomCount > 1)
    {
        angle = atan2f(map.rooms[1].centerY() - start.centerY(), map.rooms[1].centerX() - start.centerX());
    }
    else
    {
        angle = 0;
    }
    if (map.solidAt(px, py))
    {
        // The portal took the start room's middle on a one-room floor
        px -= 1.0f;
    }

    mana = maxMana();
    sealed = isBossFloor();
    exited = false;
    attackHeldMs = 0;
    attackWasDown = false;
    chargedSignalled = false;
    swingMs = castMs = hurtMs = sealedNoticeMs = 0;
    events = 0;
    populate(gen);
    rng = Rng(floorSeed(seed ^ 0xA5A5A5A5u, floor));

    floorHp = (uint16_t)hp;
    floorKills = kills;
    floorInventory = inventory;
}

bool Run::randomCell(Rng &gen, const Room &room, float &x, float &y)
{
    for (uint8_t attempt = 0; attempt < 24; attempt++)
    {
        uint8_t cx = room.x + gen.below(room.w), cy = room.y + gen.below(room.h);
        if (!map.solid(cx, cy))
        {
            x = cx + 0.5f;
            y = cy + 0.5f;
            return true;
        }
    }
    return false;
}

static uint8_t pickKind(Rng &gen, uint8_t floor)
{
    uint16_t total = 0;
    for (const EnemyDef &def : ENEMIES)
    {
        if (def.minFloor != 0 && def.minFloor <= floor)
        {
            total += def.minFloor + 1;
        }
    }
    uint16_t roll = gen.below(total);
    for (uint8_t k = 0; k < ENEMY_KIND_COUNT; k++)
    {
        const EnemyDef &def = ENEMIES[k];
        if (def.minFloor != 0 && def.minFloor <= floor)
        {
            if (roll < def.minFloor + 1)
            {
                return k;
            }
            roll -= def.minFloor + 1;
        }
    }
    return ENEMY_RAT;
}

void Run::populate(Rng &gen)
{
    float sx = px, sy = py, x, y;
    uint8_t most = 2 + floor / 3;
    most = most > 4 ? 4 : most;
    for (uint8_t r = 1; r < map.roomCount; r++)
    {
        const Room &room = map.rooms[r];
        if (r == map.portalRoom && isBossFloor() && randomCell(gen, room, x, y))
        {
            spawnEnemy(ENEMY_LICH, x, y, true);
        }
        uint8_t count = 1 + gen.below(most);
        for (uint8_t i = 0; i < count; i++)
        {
            uint8_t kind = pickKind(gen, floor);
            if (randomCell(gen, room, x, y) && fabsf(x - sx) + fabsf(y - sy) > SPAWN_CLEARANCE)
            {
                spawnEnemy(kind, x, y);
            }
        }
    }
    if (map.roomCount < 2)
    {
        return;
    }
    uint8_t items = 1 + gen.below(2);
    for (uint8_t i = 0; i < items; i++)
    {
        if (randomCell(gen, map.rooms[1 + gen.below(map.roomCount - 1)], x, y))
        {
            dropItem(x, y, rollItem(gen, floor, 0));
        }
    }
    uint8_t potions = 1 + (floor % 3 == 0);
    for (uint8_t i = 0; i < potions; i++)
    {
        if (randomCell(gen, map.rooms[1 + gen.below(map.roomCount - 1)], x, y))
        {
            dropItem(x, y, Item{}, gen.below(3) == 0 ? 2 : 1);
        }
    }
}

Enemy *Run::spawnEnemy(uint8_t kind, float x, float y, bool boss)
{
    for (Enemy &e : enemies)
    {
        if (e.state != EnemyState::DEAD)
        {
            continue;
        }
        const EnemyDef &def = ENEMIES[kind];
        float growth = 1.0f + ENEMY_HP_GROWTH * (floor - 1);
        if (boss)
        {
            growth *= 0.6f + 0.4f * (floor / BOSS_EVERY);
        }
        memset(&e, 0, sizeof(e));
        e.kind = kind;
        e.boss = boss;
        e.x = x;
        e.y = y;
        e.maxHp = (uint16_t)(def.hp * growth);
        e.hp = (int16_t)e.maxHp;
        e.damage = (uint8_t)(def.damage * (1.0f + ENEMY_DAMAGE_GROWTH * (floor - 1)));
        e.state = EnemyState::IDLE;
        return &e;
    }
    return nullptr;
}

Pickup *Run::dropItem(float x, float y, const Item &item, uint8_t potion)
{
    for (Pickup &p : pickups)
    {
        if (!p.active)
        {
            p = Pickup{x, y, item, potion, true, false};
            return &p;
        }
    }
    return nullptr;
}

void Run::dropFromBag(uint8_t index)
{
    if (index >= BAG_SIZE || inventory.bag[index].empty())
    {
        return;
    }
    Pickup *p = dropItem(px, py, inventory.bag[index]);
    if (p != nullptr)
    {
        p->waitLeave = true;
    }
    inventory.dropFromBag(index);
}

uint16_t Run::swordDamage() const
{
    uint16_t weapon = inventory.stat(SLOT_WEAPON);
    return weapon > 0 ? weapon : FIST_DAMAGE;
}

uint8_t Run::getCharge() const
{
    if (attackHeldMs < TAP_MS)
    {
        return 0;
    }
    uint32_t span = CHARGE_MS - TAP_MS, into = attackHeldMs - TAP_MS;
    return (uint8_t)(1 + (into >= span ? 254 : into * 254 / span));
}

const Enemy *Run::getBoss() const
{
    for (const Enemy &e : enemies)
    {
        if (e.boss && e.state != EnemyState::DEAD)
        {
            return &e;
        }
    }
    return nullptr;
}

void Run::clampVitals()
{
    if (hp > (int16_t)maxHp())
    {
        hp = (int16_t)maxHp();
    }
    if (mana > maxMana())
    {
        mana = maxMana();
    }
}

bool Run::usePotion(bool health)
{
    if (health)
    {
        if (inventory.hpPotions == 0 || hp >= (int16_t)maxHp())
        {
            return false;
        }
        inventory.hpPotions--;
        hp += POTION_HEAL;
    }
    else
    {
        if (inventory.mpPotions == 0 || mana >= maxMana())
        {
            return false;
        }
        inventory.mpPotions--;
        mana += POTION_MANA;
    }
    clampVitals();
    return true;
}

void Run::update(const Input &input, uint16_t dtMs)
{
    if (isDead() || exited)
    {
        return;
    }
    float dt = dtMs / 1000.0f;
    clock += dtMs;
    swingMs = tick(swingMs, dtMs);
    castMs = tick(castMs, dtMs);
    hurtMs = tick(hurtMs, dtMs);
    sealedNoticeMs = tick(sealedNoticeMs, dtMs);

    movePlayer(input, dt);
    handleAttack(input.attack, dtMs);
    mana += (MANA_REGEN + inventory.stat(SLOT_RING) / 10.0f) * dt;
    clampVitals();
    collectPickups();
    updateEnemies(dtMs);
    updateProjectiles(dtMs, dt);
    for (Spark &s : sparks)
    {
        s.timer = tick(s.timer, dtMs);
    }
}

static bool fits(const Map &map, float x, float y, float r)
{
    return !map.solidAt(x - r, y - r) && !map.solidAt(x + r, y - r) && !map.solidAt(x - r, y + r) &&
           !map.solidAt(x + r, y + r);
}

void Run::slide(float &x, float &y, float dx, float dy, float radius)
{
    if (fits(map, x + dx, y, radius))
    {
        x += dx;
    }
    if (fits(map, x, y + dy, radius))
    {
        y += dy;
    }
}

void Run::movePlayer(const Input &input, float dt)
{
    angle = wrapAngle(angle + ((input.right ? 1 : 0) - (input.left ? 1 : 0)) * PLAYER_TURN * dt);
    float step = 0;
    if (input.forward != input.back)
    {
        step = input.forward ? speed() * dt : -speed() * BACK_SPEED * dt;
    }
    if (step == 0)
    {
        return;
    }
    float dx = cosf(angle) * step, dy = sinf(angle) * step;
    slide(px, py, dx, dy, PLAYER_RADIUS);

    // Walking into the portal's block takes the player down
    float nx = px < map.portalX ? map.portalX : px > map.portalX + 1 ? map.portalX + 1 : px;
    float ny = py < map.portalY ? map.portalY : py > map.portalY + 1 ? map.portalY + 1 : py;
    if ((px - nx) * (px - nx) + (py - ny) * (py - ny) < (PLAYER_RADIUS + 0.06f) * (PLAYER_RADIUS + 0.06f))
    {
        if (!sealed)
        {
            exited = true;
            events |= EV_EXIT;
        }
        else if (sealedNoticeMs == 0)
        {
            sealedNoticeMs = SEALED_NOTICE_MS;
            events |= EV_SEALED;
        }
    }
}

void Run::handleAttack(bool down, uint16_t dtMs)
{
    if (down)
    {
        if (!attackWasDown)
        {
            attackHeldMs = 0;
        }
        else if (attackHeldMs < CHARGE_MS)
        {
            attackHeldMs = attackHeldMs + dtMs > CHARGE_MS ? CHARGE_MS : attackHeldMs + dtMs;
        }
        if (attackHeldMs >= CHARGE_MS && !chargedSignalled)
        {
            chargedSignalled = true;
            events |= EV_CHARGED;
        }
    }
    else if (attackWasDown)
    {
        if (attackHeldMs < TAP_MS)
        {
            if (swingMs == 0)
            {
                swing();
            }
        }
        else
        {
            cast((getCharge() - 1) / 254.0f);
        }
        attackHeldMs = 0;
        chargedSignalled = false;
    }
    attackWasDown = down;
}

void Run::swing()
{
    swingMs = SWING_MS;
    events |= EV_SWING;
    uint16_t damage = swordDamage();
    if (rng.below(100) < inventory.stat(SLOT_AMULET))
    {
        damage *= 2;
    }
    for (Enemy &e : enemies)
    {
        if (e.state == EnemyState::DEAD || e.state == EnemyState::DYING)
        {
            continue;
        }
        float dx = e.x - px, dy = e.y - py;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > SWING_RANGE + ENEMY_BODY * 0.5f)
        {
            continue;
        }
        if (dist > ENEMY_BODY && fabsf(wrapAngle(atan2f(dy, dx) - angle)) > SWING_ARC)
        {
            continue;
        }
        if (!map.clearLine(px, py, e.x, e.y))
        {
            continue;
        }
        float inv = dist > 0.01f ? 1.0f / dist : 0;
        hitEnemy(e, damage, dx * inv * PUSH, dy * inv * PUSH);
        addSpark(e.x - dx * inv * 0.2f, e.y - dy * inv * 0.2f);
        events |= EV_HIT;
    }
}

void Run::cast(float charge)
{
    if (mana < SPELL_COST)
    {
        events |= EV_NO_MANA;
        return;
    }
    mana -= SPELL_COST;
    castMs = CAST_ANIM_MS;
    float damage = spellDamage() * (0.6f + 0.9f * charge);
    if (rng.below(100) < inventory.stat(SLOT_AMULET))
    {
        damage *= 2;
    }
    float dx = cosf(angle), dy = sinf(angle);
    Projectile *p = fire(px + dx * SPELL_START, py + dy * SPELL_START, dx, dy, SPELL_SPEED,
                         (uint16_t)(damage + 0.5f), false);
    if (p != nullptr)
    {
        p->big = charge >= 0.999f;
    }
    events |= EV_CAST;
}

Projectile *Run::fire(float x, float y, float dirX, float dirY, float speed, uint16_t damage, bool hostile)
{
    for (Projectile &p : projectiles)
    {
        if (!p.active)
        {
            p = Projectile{x, y, dirX * speed, dirY * speed, damage, PROJECTILE_LIFE_MS, hostile, false, true};
            return &p;
        }
    }
    return nullptr;
}

void Run::addSpark(float x, float y)
{
    Spark *oldest = &sparks[0];
    for (Spark &s : sparks)
    {
        if (s.timer < oldest->timer)
        {
            oldest = &s;
        }
    }
    *oldest = Spark{x, y, SPARK_MS};
}

void Run::collectPickups()
{
    for (Pickup &p : pickups)
    {
        if (!p.active)
        {
            continue;
        }
        float d2 = (p.x - px) * (p.x - px) + (p.y - py) * (p.y - py);
        if (p.waitLeave)
        {
            if (d2 > LEAVE_REACH * LEAVE_REACH)
            {
                p.waitLeave = false;
            }
            continue;
        }
        if (d2 > PICK_REACH * PICK_REACH)
        {
            continue;
        }
        if (p.potion != 0)
        {
            uint8_t &count = p.potion == 1 ? inventory.hpPotions : inventory.mpPotions;
            lastPotion = p.potion;
            if (count >= MAX_POTIONS)
            {
                p.waitLeave = true;
                lastPicked = Item{};
                events |= EV_BAG_FULL;
                continue;
            }
            count++;
            p.active = false;
            events |= EV_POTION;
            continue;
        }
        uint16_t before = maxHp();
        Item displaced;
        lastPicked = p.item;
        lastPotion = 0;
        switch (inventory.pickUp(p.item, displaced))
        {
        case Pick::EQUIPPED:
            events |= EV_EQUIP;
            break;
        case Pick::BAGGED:
            events |= EV_BAG;
            break;
        case Pick::BAG_FULL:
            events |= EV_BAG_FULL;
            break;
        }
        if (maxHp() > before)
        {
            hp += maxHp() - before;
        }
        clampVitals();
        if (displaced.empty())
        {
            p.active = false;
        }
        else
        {
            p.item = displaced;
            p.waitLeave = true;
        }
    }
}

void Run::hurtPlayer(uint16_t damage)
{
    if (isDead())
    {
        return;
    }
    uint16_t armor = inventory.stat(SLOT_ARMOR);
    damage = damage > armor + 1 ? damage - armor : 1;
    hp -= (int16_t)damage;
    hurtMs = HURT_FLASH_MS;
    events |= EV_HURT;
    if (hp <= 0)
    {
        hp = 0;
        events |= EV_DIED;
    }
}

void Run::hitEnemy(Enemy &e, uint16_t damage, float pushX, float pushY)
{
    e.hp -= (int16_t)damage;
    e.flashMs = 120;
    if (e.hp <= 0)
    {
        killEnemy(e);
        return;
    }
    if (!e.boss)
    {
        slide(e.x, e.y, pushX, pushY, ENEMY_RADIUS);
        e.state = EnemyState::HURT;
        e.timer = HURT_STAGGER_MS;
    }
    else if (e.state == EnemyState::IDLE)
    {
        e.state = EnemyState::CHASE;
    }
}

void Run::killEnemy(Enemy &e)
{
    e.hp = 0;
    e.state = EnemyState::DYING;
    e.timer = DYING_MS;
    if (kills < UINT16_MAX)
    {
        kills++;
    }
    events |= EV_KILL;
    if (e.boss)
    {
        sealed = false;
        events |= EV_BOSS_DOWN;
        dropItem(e.x, e.y, rollItem(rng, floor, 1));
        dropItem(e.x + 0.3f, e.y + 0.2f, Item{}, 1);
        return;
    }
    if (rng.below(100) < DROP_ITEM)
    {
        dropItem(e.x, e.y, rollItem(rng, floor, 0));
    }
    else if (rng.below(100) < DROP_POTION)
    {
        dropItem(e.x, e.y, Item{}, rng.below(10) < 7 ? 1 : 2);
    }
}

void Run::updateEnemies(uint16_t dtMs)
{
    float dt = dtMs / 1000.0f;
    for (Enemy &e : enemies)
    {
        if (e.state != EnemyState::DEAD)
        {
            updateEnemy(e, dtMs, dt);
        }
    }
    // Keeps a pack from melting into one sprite
    for (uint8_t i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy &a = enemies[i];
        if (a.state == EnemyState::DEAD || a.state == EnemyState::DYING)
        {
            continue;
        }
        for (uint8_t j = i + 1; j < MAX_ENEMIES; j++)
        {
            Enemy &b = enemies[j];
            if (b.state == EnemyState::DEAD || b.state == EnemyState::DYING)
            {
                continue;
            }
            float dx = b.x - a.x, dy = b.y - a.y, d2 = dx * dx + dy * dy;
            if (d2 < 0.36f && d2 > 0.0001f)
            {
                float d = sqrtf(d2), push = (0.6f - d) * 0.5f / d;
                slide(a.x, a.y, -dx * push, -dy * push, ENEMY_RADIUS);
                slide(b.x, b.y, dx * push, dy * push, ENEMY_RADIUS);
            }
        }
    }
}

void Run::updateEnemy(Enemy &e, uint16_t dtMs, float dt)
{
    const EnemyDef &def = ENEMIES[e.kind];
    e.flashMs = tick(e.flashMs, dtMs);
    if (e.state == EnemyState::DYING)
    {
        e.timer = tick(e.timer, dtMs);
        if (e.timer == 0)
        {
            e.state = EnemyState::DEAD;
        }
        return;
    }
    float dx = px - e.x, dy = py - e.y;
    float dist = sqrtf(dx * dx + dy * dy);
    switch (e.state)
    {
    case EnemyState::IDLE:
        if (dist < SIGHT_RANGE && map.clearLine(e.x, e.y, px, py))
        {
            e.state = EnemyState::CHASE;
            events |= EV_ALERT;
        }
        break;
    case EnemyState::HURT:
        e.timer = tick(e.timer, dtMs);
        if (e.timer == 0)
        {
            e.state = EnemyState::CHASE;
        }
        break;
    case EnemyState::CHASE:
    {
        bool sees = map.clearLine(e.x, e.y, px, py);
        bool inReach = dist <= def.reach && (sees || !def.ranged);
        if (inReach)
        {
            e.state = EnemyState::WINDUP;
            e.timer = def.windupMs;
            break;
        }
        float step = def.speed * dt;
        if (dist - step > ENEMY_GAP && dist > 0.01f)
        {
            slide(e.x, e.y, dx / dist * step, dy / dist * step, ENEMY_RADIUS);
        }
        break;
    }
    case EnemyState::WINDUP:
        e.timer = tick(e.timer, dtMs);
        if (e.timer == 0)
        {
            if (def.ranged)
            {
                if (dist > 0.01f && map.clearLine(e.x, e.y, px, py))
                {
                    fire(e.x + dx / dist * 0.3f, e.y + dy / dist * 0.3f, dx / dist, dy / dist, BOLT_SPEED, e.damage,
                         true);
                    events |= EV_ENEMY_SHOT;
                }
            }
            else if (dist <= def.reach + 0.35f)
            {
                hurtPlayer(e.damage);
            }
            e.state = EnemyState::REST;
            e.timer = e.boss && e.hp < e.maxHp / 2 ? def.restMs / 2 : def.restMs;
        }
        break;
    case EnemyState::REST:
        e.timer = tick(e.timer, dtMs);
        if (e.timer == 0)
        {
            e.state = EnemyState::CHASE;
        }
        break;
    default:
        break;
    }
}

void Run::updateProjectiles(uint16_t dtMs, float dt)
{
    // Short steps, so a fast spell cannot pass through a thin enemy between two frames
    const uint8_t STEPS = 3;
    for (Projectile &p : projectiles)
    {
        if (!p.active)
        {
            continue;
        }
        p.lifeMs = tick(p.lifeMs, dtMs);
        if (p.lifeMs == 0)
        {
            p.active = false;
            continue;
        }
        for (uint8_t s = 0; s < STEPS && p.active; s++)
        {
            p.x += p.dx * dt / STEPS;
            p.y += p.dy * dt / STEPS;
            Enemy *struck = nullptr;
            if (map.solidAt(p.x, p.y))
            {
                p.x -= p.dx * dt / STEPS;
                p.y -= p.dy * dt / STEPS;
            }
            else if (p.hostile)
            {
                float d2 = (p.x - px) * (p.x - px) + (p.y - py) * (p.y - py);
                if (d2 < 0.35f * 0.35f)
                {
                    hurtPlayer(p.damage);
                    p.active = false;
                    addSpark(p.x, p.y);
                }
                continue;
            }
            else
            {
                for (Enemy &e : enemies)
                {
                    if (e.state == EnemyState::DEAD || e.state == EnemyState::DYING)
                    {
                        continue;
                    }
                    float d2 = (p.x - e.x) * (p.x - e.x) + (p.y - e.y) * (p.y - e.y);
                    if (d2 < ENEMY_BODY * ENEMY_BODY)
                    {
                        struck = &e;
                        break;
                    }
                }
                if (struck == nullptr)
                {
                    continue;
                }
                float len = sqrtf(p.dx * p.dx + p.dy * p.dy);
                hitEnemy(*struck, p.damage, p.dx / len * PUSH, p.dy / len * PUSH);
                events |= EV_HIT;
            }
            // It hit a wall or an enemy
            p.active = false;
            addSpark(p.x, p.y);
            if (p.big)
            {
                events |= EV_BLAST;
                for (Enemy &e : enemies)
                {
                    if (&e == struck || e.state == EnemyState::DEAD || e.state == EnemyState::DYING)
                    {
                        continue;
                    }
                    float ex = e.x - p.x, ey = e.y - p.y, d = sqrtf(ex * ex + ey * ey);
                    if (d < BLAST_RADIUS)
                    {
                        hitEnemy(e, p.damage * 6 / 10, d > 0.01f ? ex / d * PUSH : 0, d > 0.01f ? ey / d * PUSH : 0);
                    }
                }
            }
        }
    }
}
