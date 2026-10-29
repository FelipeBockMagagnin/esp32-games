#ifndef ROGUE_RUN_H
#define ROGUE_RUN_H

#include "Config.h"
#include "Map.h"

enum class Action : uint8_t
{
    NORTH,
    SOUTH,
    WEST,
    EAST,
    WAIT,
    DRINK,
    DESCEND
};

// What one act() did, as bits, for the sound and the screen to react to
enum Event : uint16_t
{
    EV_NONE = 0,
    EV_BLOCKED = 1 << 0, // Nothing happened and no turn passed
    EV_MOVED = 1 << 1,
    EV_HIT = 1 << 2,
    EV_KILL = 1 << 3,
    EV_HURT = 1 << 4,
    EV_PICKUP = 1 << 5,
    EV_GEAR = 1 << 6, // Wore something better
    EV_HEAL = 1 << 7,
    EV_LEVEL_UP = 1 << 8,
    EV_FLOOR = 1 << 9,
    EV_DIED = 1 << 10,
};

struct Entity
{
    enum Kind : uint8_t
    {
        NONE,
        ENEMY,
        ITEM
    };

    Kind kind;
    uint8_t type; // Index into ENEMIES or ITEMS
    uint8_t x, y;
    int16_t hp;     // Enemies; the gold amount for a pile of gold
    bool awake;
};

// A run is deterministic from its seed and the player's actions: a floor is generated from
// the run seed and its number alone, and nothing else rolls dice, so a save only needs what
// the player carries into a floor.
class Run
{
public:
    static const uint8_t MAX_ENTITIES = 16;
    static const uint8_t MESSAGE_LINES = 2;
    static const uint8_t MESSAGE_LEN = 38; // What fits across the HUD at size 1

    // POD, for the save. A zeroed snapshot is no run.
    struct Snapshot
    {
        uint32_t seed;
        uint32_t gold;
        uint16_t xp;
        uint16_t kills;
        int16_t hp;
        uint8_t floor; // 0: no run
        uint8_t level;
        uint8_t potions;
        uint8_t weapon; // ItemId + 1, 0 for none
        uint8_t armor;
    };

    Run();

    void start(uint32_t seed);
    bool isActive() const { return floor > 0; }
    bool isDead() const { return hp <= 0; }

    uint16_t act(Action action);

    // The start of the current floor, which is where a loaded run resumes
    void save(Snapshot &out) const { out = floorStart; }
    void load(const Snapshot &in);

    const Map &getMap() const { return map; }
    const Entity &getEntity(uint8_t i) const { return entities[i]; }
    uint8_t getPlayerX() const { return px; }
    uint8_t getPlayerY() const { return py; }
    bool onStairs() const { return map.at(px, py) == TILE_STAIRS; }

    int16_t getHp() const { return hp; }
    int16_t getMaxHp() const { return PLAYER_BASE_HP + (level - 1) * HP_PER_LEVEL; }
    int16_t getAttack() const;
    int16_t getDefense() const;
    uint8_t getLevel() const { return level; }
    uint16_t getXp() const { return xp; }
    uint8_t getFloor() const { return floor; }
    uint8_t getPotions() const { return potions; }
    // ItemId + 1 of what is worn, 0 for nothing
    uint8_t getWeapon() const { return weapon; }
    uint8_t getArmor() const { return armor; }
    uint32_t getGold() const { return gold; }
    uint16_t getKills() const { return kills; }
    uint32_t getScore() const;
    // ENEMY_COUNT when nothing has killed the player
    uint8_t getKiller() const { return killer; }
    // The turn's log: the player's action, then what the enemies did
    const char *getMessage(uint8_t line) const { return messages[line]; }
    // Bumped by every act() that changed anything, so the screen knows to look
    uint16_t getRevision() const { return revision; }

private:
    Map map;
    Entity entities[MAX_ENTITIES];
    Snapshot floorStart;

    uint32_t seed;
    uint32_t gold;
    uint16_t xp;
    uint16_t kills;
    uint16_t turn;
    uint16_t revision;
    int16_t hp;
    uint8_t floor;
    uint8_t level;
    uint8_t potions;
    uint8_t weapon;
    uint8_t armor;
    uint8_t px, py;
    uint8_t killer;
    char messages[MESSAGE_LINES][MESSAGE_LEN + 1];

    void enterFloor();
    void populate(Rng &rng);
    bool placeFree(Rng &rng, uint8_t &x, uint8_t &y, bool awayFromStart);
    int8_t entityAt(uint8_t x, uint8_t y, Entity::Kind kind) const;

    uint16_t moveBy(int8_t dx, int8_t dy);
    uint16_t attack(Entity &enemy);
    uint16_t pickUp(Entity &item);
    uint16_t gainXp(uint16_t amount);
    uint16_t enemiesAct();
    void say(const char *format, ...);
};

#endif // ROGUE_RUN_H
