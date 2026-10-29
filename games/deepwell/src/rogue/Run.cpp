#include "Run.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const uint8_t PLACE_ATTEMPTS = 100;
static const uint8_t SAFE_DISTANCE = 4; // Steps from the start an enemy or item spawns at least
static const uint8_t STARTING_POTIONS = 2;
static const int16_t MIN_POTION_HEAL = 8;

static uint8_t distance(int16_t ax, int16_t ay, int16_t bx, int16_t by)
{
    int16_t dx = ax - bx, dy = ay - by;
    return (uint8_t)((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
}

Run::Run()
    : floorStart(), seed(0), gold(0), xp(0), kills(0), turn(0), revision(0), hp(0), floor(0), level(1),
      potions(0), weapon(0), armor(0), px(0), py(0), killer(ENEMY_COUNT), messages()
{
    memset(entities, 0, sizeof(entities));
    memset(&map, 0, sizeof(map));
}

void Run::start(uint32_t runSeed)
{
    Snapshot fresh = {};
    fresh.seed = runSeed;
    fresh.floor = 1;
    fresh.level = 1;
    fresh.potions = STARTING_POTIONS;
    fresh.hp = PLAYER_BASE_HP;
    load(fresh);
    say("Find the stairs down.");
}

void Run::load(const Snapshot &in)
{
    seed = in.seed;
    gold = in.gold;
    xp = in.xp;
    kills = in.kills;
    floor = in.floor;
    level = in.level > 0 ? in.level : 1;
    potions = in.potions;
    weapon = in.weapon <= ITEM_COUNT ? in.weapon : 0;
    armor = in.armor <= ITEM_COUNT ? in.armor : 0;
    hp = in.hp < getMaxHp() ? in.hp : getMaxHp();
    killer = ENEMY_COUNT;
    memset(messages, 0, sizeof(messages));
    if (floor == 0)
    {
        revision++;
        return;
    }
    enterFloor();
}

void Run::enterFloor()
{
    Rng rng(seed ^ (floor * 0x9E3779B9u));
    map.generate(rng);
    px = map.startX;
    py = map.startY;
    memset(entities, 0, sizeof(entities));
    populate(rng);
    turn = 0;

    floorStart.seed = seed;
    floorStart.gold = gold;
    floorStart.xp = xp;
    floorStart.kills = kills;
    floorStart.hp = hp;
    floorStart.floor = floor;
    floorStart.level = level;
    floorStart.potions = potions;
    floorStart.weapon = weapon;
    floorStart.armor = armor;
    revision++;
}

bool Run::placeFree(Rng &rng, uint8_t &x, uint8_t &y, bool awayFromStart)
{
    for (uint8_t attempt = 0; attempt < PLACE_ATTEMPTS; attempt++)
    {
        const Room &room = map.rooms[rng.below(map.roomCount)];
        uint8_t cx = room.x + rng.below(room.w);
        uint8_t cy = room.y + rng.below(room.h);
        // The distance rule gives way in the last tries, for a floor that is one small room
        bool farEnough = !awayFromStart || attempt >= PLACE_ATTEMPTS / 2 ||
                         distance(cx, cy, map.startX, map.startY) >= SAFE_DISTANCE;
        if (map.at(cx, cy) == TILE_FLOOR && (cx != px || cy != py) && farEnough &&
            entityAt(cx, cy, Entity::ENEMY) < 0 && entityAt(cx, cy, Entity::ITEM) < 0)
        {
            x = cx;
            y = cy;
            return true;
        }
    }
    return false;
}

void Run::populate(Rng &rng)
{
    uint8_t slot = 0;

    uint8_t eligible = 0;
    while (eligible < ENEMY_COUNT && ENEMIES[eligible].minFloor <= floor)
    {
        eligible++;
    }
    // The three newest kinds the floor allows, so early ones fade out as the run goes deeper
    uint8_t oldest = eligible > 3 ? eligible - 3 : 0;
    for (uint8_t i = 0; i < enemiesOnFloor(floor) && slot < MAX_ENTITIES; i++)
    {
        Entity &e = entities[slot];
        if (!placeFree(rng, e.x, e.y, true))
        {
            break;
        }
        e.kind = Entity::ENEMY;
        e.type = (uint8_t)rng.range(oldest, eligible - 1);
        e.hp = enemyStat(ENEMIES[e.type].hp, e.type, floor);
        e.awake = false;
        slot++;
    }

    for (uint8_t i = 0; i < itemsOnFloor(floor) && slot < MAX_ENTITIES; i++)
    {
        Entity &e = entities[slot];
        if (!placeFree(rng, e.x, e.y, false))
        {
            break;
        }
        e.kind = Entity::ITEM;
        uint32_t roll = rng.below(100);
        if (roll < 45)
        {
            e.type = ITEM_POTION;
        }
        else if (roll < 75)
        {
            e.type = ITEM_GOLD;
            e.hp = (int16_t)rng.range(3, 5 + floor * 3);
        }
        else
        {
            // One of the two best pieces of that kind the floor allows
            ItemKind kind = rng.below(2) == 0 ? ItemKind::WEAPON : ItemKind::ARMOR;
            uint8_t best = 0, second = 0;
            for (uint8_t id = 0; id < ITEM_COUNT; id++)
            {
                if (ITEMS[id].kind == kind && ITEMS[id].minFloor <= floor)
                {
                    second = best != 0 ? best : id;
                    best = id;
                }
            }
            e.type = rng.below(2) == 0 ? best : second;
        }
        slot++;
    }
}

int8_t Run::entityAt(uint8_t x, uint8_t y, Entity::Kind kind) const
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entities[i].kind == kind && entities[i].x == x && entities[i].y == y)
        {
            return (int8_t)i;
        }
    }
    return -1;
}

int16_t Run::getAttack() const
{
    return PLAYER_BASE_ATTACK + (level - 1) + (weapon > 0 ? ITEMS[weapon - 1].value : 0);
}

int16_t Run::getDefense() const
{
    return PLAYER_BASE_DEFENSE + (level - 1) / 2 + (armor > 0 ? ITEMS[armor - 1].value : 0);
}

uint32_t Run::getScore() const
{
    uint8_t below = floor > 0 ? floor - 1 : 0;
    return gold + (uint32_t)SCORE_PER_FLOOR * below + (uint32_t)SCORE_PER_KILL * kills;
}

// Fills the first empty log line, or runs on after the last one
void Run::say(const char *format, ...)
{
    uint8_t line = 0;
    while (line < MESSAGE_LINES - 1 && messages[line][0] != '\0')
    {
        line++;
    }
    char *text = messages[line];
    size_t used = strlen(text);
    if (used > 0 && used < MESSAGE_LEN)
    {
        text[used++] = ' ';
        text[used] = '\0';
    }
    if (used >= MESSAGE_LEN)
    {
        return;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(text + used, MESSAGE_LEN + 1 - used, format, args);
    va_end(args);
}

uint16_t Run::act(Action action)
{
    if (!isActive() || isDead())
    {
        return EV_BLOCKED;
    }
    memset(messages, 0, sizeof(messages));
    revision++;

    uint16_t events = EV_NONE;
    switch (action)
    {
    case Action::NORTH:
        events = moveBy(0, -1);
        break;
    case Action::SOUTH:
        events = moveBy(0, 1);
        break;
    case Action::WEST:
        events = moveBy(-1, 0);
        break;
    case Action::EAST:
        events = moveBy(1, 0);
        break;
    case Action::WAIT:
        say("You wait.");
        break;
    case Action::DRINK:
    {
        if (potions == 0)
        {
            say("No potions left.");
            return EV_BLOCKED;
        }
        if (hp >= getMaxHp())
        {
            say("Already at full health.");
            return EV_BLOCKED;
        }
        int16_t heal = getMaxHp() / 2 > MIN_POTION_HEAL ? getMaxHp() / 2 : MIN_POTION_HEAL;
        int16_t before = hp;
        hp = hp + heal < getMaxHp() ? hp + heal : getMaxHp();
        potions--;
        say("You drink a potion (+%d HP).", hp - before);
        events = EV_HEAL;
        break;
    }
    case Action::DESCEND:
        if (!onStairs())
        {
            say("There are no stairs here.");
            return EV_BLOCKED;
        }
        floor++;
        enterFloor();
        say("Floor %u.", (unsigned)floor);
        return EV_FLOOR;
    }

    if (events & EV_BLOCKED)
    {
        return events;
    }

    turn++;
    if (turn % REGEN_TURNS == 0 && hp < getMaxHp())
    {
        hp++;
    }
    return events | enemiesAct();
}

uint16_t Run::moveBy(int8_t dx, int8_t dy)
{
    int16_t nx = px + dx, ny = py + dy;
    int8_t enemy = entityAt(nx, ny, Entity::ENEMY);
    if (enemy >= 0)
    {
        return attack(entities[enemy]);
    }
    if (!map.walkable(nx, ny))
    {
        return EV_BLOCKED;
    }

    px = (uint8_t)nx;
    py = (uint8_t)ny;
    uint16_t events = EV_MOVED;
    int8_t item = entityAt(px, py, Entity::ITEM);
    if (item >= 0)
    {
        events |= pickUp(entities[item]);
    }
    if (onStairs())
    {
        say("Stairs down: Confirm to descend.");
    }
    return events;
}

uint16_t Run::attack(Entity &enemy)
{
    const EnemyDef &def = ENEMIES[enemy.type];
    int16_t damage = damageThrough(getAttack(), def.defense);
    enemy.hp -= damage;
    enemy.awake = true;
    if (enemy.hp > 0)
    {
        say("You hit the %s (%d).", def.name, damage);
        return EV_HIT;
    }
    enemy.kind = Entity::NONE;
    kills++;
    say("You slay the %s.", def.name);
    return EV_HIT | EV_KILL | gainXp((uint16_t)enemyStat(def.xp, enemy.type, floor));
}

uint16_t Run::gainXp(uint16_t amount)
{
    xp += amount;
    uint16_t events = EV_NONE;
    while (level < 99 && xp >= xpForLevel(level))
    {
        level++;
        hp += HP_PER_LEVEL;
        events = EV_LEVEL_UP;
    }
    if (events)
    {
        say("Level %u!", (unsigned)level);
    }
    return events;
}

uint16_t Run::pickUp(Entity &item)
{
    const ItemDef &def = ITEMS[item.type];
    switch (def.kind)
    {
    case ItemKind::POTION:
        if (potions >= MAX_POTIONS)
        {
            say("No room for another potion.");
            return EV_NONE;
        }
        potions++;
        say("You pick up a potion.");
        break;
    case ItemKind::GOLD:
        gold += (uint32_t)item.hp;
        say("You find %d gold.", item.hp);
        break;
    case ItemKind::WEAPON:
    case ItemKind::ARMOR:
    {
        uint8_t &worn = def.kind == ItemKind::WEAPON ? weapon : armor;
        int16_t current = worn > 0 ? ITEMS[worn - 1].value : 0;
        if (def.value > current)
        {
            worn = item.type + 1;
            item.kind = Entity::NONE;
            say("%s: +%d %s.", def.name, def.value, def.kind == ItemKind::WEAPON ? "attack" : "defense");
            return EV_PICKUP | EV_GEAR;
        }
        uint32_t scrap = (uint32_t)def.value * SCRAP_GOLD_PER_POINT;
        gold += scrap;
        say("Scrapped a %s for %u gold.", def.name, (unsigned)scrap);
        break;
    }
    }
    item.kind = Entity::NONE;
    return EV_PICKUP;
}

uint16_t Run::enemiesAct()
{
    uint16_t events = EV_NONE;
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        Entity &e = entities[i];
        if (e.kind != Entity::ENEMY)
        {
            continue;
        }
        uint8_t steps = distance(e.x, e.y, px, py);
        if (!e.awake)
        {
            // Woken this turn, it moves from the next one, so the player sees it coming
            e.awake = steps <= WAKE_DISTANCE;
            continue;
        }

        const EnemyDef &def = ENEMIES[e.type];
        if (steps == 1)
        {
            int16_t damage = damageThrough(enemyStat(def.attack, e.type, floor), getDefense());
            hp -= damage;
            events |= EV_HURT;
            if (hp <= 0)
            {
                hp = 0;
                killer = e.type;
                say("The %s kills you.", def.name);
                return events | EV_DIED;
            }
            say("The %s hits you (%d).", def.name, damage);
            continue;
        }

        // Greedy: the longer axis first, the other if that way is blocked
        int16_t dx = (int16_t)px - e.x, dy = (int16_t)py - e.y;
        int8_t sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
        int8_t sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
        bool horizontal = (dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy);
        for (uint8_t attempt = 0; attempt < 2; attempt++, horizontal = !horizontal)
        {
            int16_t nx = e.x + (horizontal ? sx : 0);
            int16_t ny = e.y + (horizontal ? 0 : sy);
            if ((nx != e.x || ny != e.y) && map.walkable(nx, ny) && entityAt(nx, ny, Entity::ENEMY) < 0 &&
                (nx != px || ny != py))
            {
                e.x = (uint8_t)nx;
                e.y = (uint8_t)ny;
                break;
            }
        }
    }
    return events;
}
