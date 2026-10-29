#ifndef CRAWL_RUN_H
#define CRAWL_RUN_H

#include "Config.h"
#include "Items.h"
#include "Map.h"
#include "Rng.h"

struct Input
{
    bool forward;
    bool back;
    bool left;
    bool right;
    bool attack; // Confirm, held
};

enum Event : uint32_t
{
    EV_SWING = 1u << 0,
    EV_HIT = 1u << 1,
    EV_KILL = 1u << 2,
    EV_CAST = 1u << 3,
    EV_NO_MANA = 1u << 4,
    EV_HURT = 1u << 5,
    EV_EQUIP = 1u << 6,
    EV_BAG = 1u << 7,
    EV_BAG_FULL = 1u << 8,
    EV_POTION = 1u << 9,
    EV_EXIT = 1u << 10,
    EV_DIED = 1u << 11,
    EV_ALERT = 1u << 12,
    EV_ENEMY_SHOT = 1u << 13,
    EV_SEALED = 1u << 14,
    EV_BOSS_DOWN = 1u << 15,
    EV_CHARGED = 1u << 16,
    EV_BLAST = 1u << 17,
};

enum class EnemyState : uint8_t
{
    IDLE,
    CHASE,
    WINDUP,
    REST,
    HURT,
    DYING,
    DEAD
};

struct Enemy
{
    float x, y;
    int16_t hp;
    uint16_t maxHp;
    uint16_t timer;
    uint16_t flashMs;
    uint8_t damage;
    uint8_t kind;
    EnemyState state;
    bool boss;
};

struct Pickup
{
    float x, y;
    Item item;
    uint8_t potion; // 0 gear, 1 health, 2 mana
    bool active;
    bool waitLeave; // Refused once: only taken again after the player steps off it
};

struct Projectile
{
    float x, y, dx, dy;
    uint16_t damage;
    uint16_t lifeMs;
    bool hostile;
    bool big; // A full charge: bursts over everything near where it lands
    bool active;
};

struct Spark
{
    float x, y;
    uint16_t timer;
};

class Run
{
public:
    static const uint8_t MAX_ENEMIES = 24;
    static const uint8_t MAX_PICKUPS = 24;
    static const uint8_t MAX_PROJECTILES = 12;
    static const uint8_t MAX_SPARKS = 6;
    static const uint16_t SPARK_MS = 220;
    static const uint16_t DYING_MS = 380;

    // What a save keeps: the run as it stood when the floor began
    struct Snapshot
    {
        uint32_t seed;
        uint16_t hp;
        uint16_t kills;
        uint8_t floor; // 0 no run
        uint8_t pad[3];
        Inventory inventory;
    };

    void newRun(uint32_t seed);
    void restore(const Snapshot &snapshot);
    void save(Snapshot &out) const;
    void nextFloor();

    void update(const Input &input, uint16_t dtMs);
    uint32_t takeEvents()
    {
        uint32_t e = events;
        events = 0;
        return e;
    }
    bool usePotion(bool health);
    void equipFromBag(uint8_t index) { inventory.equipFromBag(index); clampVitals(); }
    // Back on the floor at the player's feet, picked up again only after stepping off
    void dropFromBag(uint8_t index);
    // Gives up the run: saved from now on as no run
    void end() { hp = 0; }

    // Derived from the gear
    uint16_t maxHp() const { return PLAYER_HP + inventory.stat(SLOT_HELM); }
    uint16_t maxMana() const { return PLAYER_MANA; }
    uint16_t swordDamage() const;
    uint16_t spellDamage() const { return SPELL_BASE + inventory.stat(SLOT_FOCUS); }
    float speed() const { return PLAYER_SPEED * (1.0f + inventory.stat(SLOT_BOOTS) / 100.0f); }

    const Map &getMap() const { return map; }
    float getX() const { return px; }
    float getY() const { return py; }
    float getAngle() const { return angle; }
    int16_t getHp() const { return hp; }
    uint16_t getMana() const { return (uint16_t)mana; }
    uint8_t getFloor() const { return floor; }
    uint16_t getKills() const { return kills; }
    bool isDead() const { return hp <= 0; }
    bool isBossFloor() const { return floor % BOSS_EVERY == 0; }
    bool isSealed() const { return sealed; }
    // 0 when not charging, else how far the charge got, 1 to 255
    uint8_t getCharge() const;
    bool isCharging() const { return attackHeldMs >= TAP_MS; }
    uint16_t getSwingMs() const { return swingMs; }
    uint16_t getCastMs() const { return castMs; }
    uint16_t getHurtMs() const { return hurtMs; }
    uint32_t getClock() const { return clock; }
    const Enemy *getBoss() const;
    const Item &getLastPicked() const { return lastPicked; }
    uint8_t getLastPotion() const { return lastPotion; }

    const Enemy *getEnemies() const { return enemies; }
    const Pickup *getPickups() const { return pickups; }
    const Projectile *getProjectiles() const { return projectiles; }
    const Spark *getSparks() const { return sparks; }

    Inventory inventory;

    // For the tests and the PC shots
    void place(float x, float y, float a) { px = x, py = y, angle = a; }
    Enemy *spawnEnemy(uint8_t kind, float x, float y, bool boss = false);
    Pickup *dropItem(float x, float y, const Item &item, uint8_t potion = 0);
    Enemy *enemyAt(uint8_t i) { return &enemies[i]; }

private:
    Map map;
    Rng rng = Rng(1);
    uint32_t seed = 0;
    uint32_t events = 0;
    uint32_t clock = 0;
    float px = 0, py = 0, angle = 0;
    int16_t hp = 0;
    float mana = 0;
    uint16_t kills = 0;
    uint8_t floor = 0;
    uint16_t floorHp = 0;
    uint16_t floorKills = 0;
    Inventory floorInventory;
    bool sealed = false;
    bool exited = false;
    bool chargedSignalled = false;

    uint16_t attackHeldMs = 0;
    bool attackWasDown = false;
    uint16_t swingMs = 0;
    uint16_t castMs = 0;
    uint16_t hurtMs = 0;
    uint16_t sealedNoticeMs = 0;
    Item lastPicked = {};
    uint8_t lastPotion = 0;

    Enemy enemies[MAX_ENEMIES];
    Pickup pickups[MAX_PICKUPS];
    Projectile projectiles[MAX_PROJECTILES];
    Spark sparks[MAX_SPARKS];

    void startFloor();
    void populate(Rng &gen);
    void clampVitals();
    bool randomCell(Rng &gen, const Room &room, float &x, float &y);

    void movePlayer(const Input &input, float dt);
    void slide(float &x, float &y, float dx, float dy, float radius);
    void handleAttack(bool down, uint16_t dtMs);
    void swing();
    void cast(float charge);
    void collectPickups();
    void updateEnemies(uint16_t dtMs);
    void updateEnemy(Enemy &e, uint16_t dtMs, float dt);
    void updateProjectiles(uint16_t dtMs, float dt);
    void hitEnemy(Enemy &e, uint16_t damage, float pushX, float pushY);
    void hurtPlayer(uint16_t damage);
    void killEnemy(Enemy &e);
    void addSpark(float x, float y);
    Projectile *fire(float x, float y, float dirX, float dirY, float speed, uint16_t damage, bool hostile);
};

#endif // CRAWL_RUN_H
