#ifndef NAN_GAME_H
#define NAN_GAME_H

#include <stdint.h>
#include "Config.h"

// The whole game state and its rules, with no Arduino or display in sight, so the tests and
// the balance simulator in tools/ compile it for the PC. Everything moves through tick(),
// which main.cpp calls with a fixed step whatever screen is showing.
class Game
{
public:
    // Plain data, saved as raw bytes by SlotStore: numbers and table indices only
    struct Snapshot
    {
        uint8_t version;
        uint8_t zone;
        uint8_t fighting;
        int8_t equipped[EQUIP_COUNT];
        uint16_t unlocked;     // A bit per Feature
        uint16_t bossesBeaten; // This run
        uint16_t bestBosses;   // Any run
        uint16_t rebirths;
        uint16_t xpBought[(uint8_t)XpUpgrade::COUNT];
        uint16_t miners[MINER_COUNT];
        uint16_t augmentLevel[AUGMENT_COUNT];
        uint8_t itemLevel[ITEM_COUNT];
        uint32_t skillLevel[SKILL_COUNT];
        float skillProgress[SKILL_COUNT];
        float skillAlloc[SKILL_COUNT];
        float augmentProgress[AUGMENT_COUNT];
        float augmentAlloc[AUGMENT_COUNT];
        float energy;
        float bossHp;
        float bossPlayerHp;
        float advHp;
        uint32_t nanPoints;
        uint32_t runSeconds;
        uint32_t playSeconds;
        double xp;
        double gold;
    };
    static const uint8_t SNAPSHOT_VERSION = 1;

    // Allocation targets: training skills, then augments
    struct Target
    {
        bool augment;
        uint8_t index;
    };

    enum class EnemyState : uint8_t
    {
        NONE,    // Safe zone
        RESPAWN, // Waiting for the next one
        FIGHTING
    };

    explicit Game(uint32_t (*random)());

    void newGame();
    void capture(Snapshot &out) const;
    // False leaves a new game in place: wrong version, or values out of range
    bool restore(const Snapshot &in);

    void tick(double dt);

    // ---- Energy
    double energyCap() const;
    double energyRegen() const;
    double energyPower() const;
    double energy() const { return energyNow; }
    double allocated() const;
    double idleEnergy() const { return energyNow - allocated(); }
    double allocStep() const;
    double allocatedTo(Target t) const;
    // Both clamp to what is idle (allocate) or allocated there (reclaim) and return the amount moved
    double allocate(Target t, double amount);
    double reclaim(Target t, double amount);
    void reclaimAll();

    // ---- Training
    bool skillOpen(uint8_t i) const;
    uint32_t skillLevel(uint8_t i) const { return skills[i].level; }
    double skillProgress(uint8_t i) const; // 0..1 toward the next level
    double skillSecondsPerLevel(uint8_t i) const; // 0 with nothing allocated
    double skillStat(uint8_t i) const;

    // ---- The player's stats, shared by the boss fight and the adventure
    double attack() const;
    double defense() const;
    double maxHp() const { return attack() * HP_PER_ATTACK; }
    double hpRegen() const { return defense() / DEFENSE_PER_REGEN; }
    double gearPercent(bool attackStat) const;
    double augmentPercent(AugmentEffect effect) const;
    double rebirthPercent() const { return nanPoints * REBIRTH_PERCENT; }

    // ---- Boss ladder
    uint16_t bossesBeaten() const { return beaten; }
    uint16_t bestBossesBeaten() const { return bestBeaten; }
    static const BossDef &bossDef(uint16_t index) { return BOSSES[index % BOSS_TABLE_COUNT]; }
    static uint16_t bossLap(uint16_t index) { return index / BOSS_TABLE_COUNT; }
    static double bossAttack(uint16_t index);
    static double bossDefense(uint16_t index);
    static double bossMaxHp(uint16_t index);
    static double bossXp(uint16_t index);
    double bossHp() const { return bossHpNow; }
    double bossPlayerHp() const { return bossPlayerHpNow; }
    bool isFighting() const { return fighting; }
    // Refuses to start with the player's HP under a tenth, which would only lose again
    bool setFighting(bool on);
    double bossDps() const;   // The player's damage to the boss
    double bossHurts() const; // The boss's damage to the player

    // ---- Spend XP
    double xp() const { return xpNow; }
    uint16_t xpBought(XpUpgrade u) const { return xpBoughtCount[(uint8_t)u]; }
    double xpCost(XpUpgrade u) const;
    bool buyXp(XpUpgrade u);
    double luck() const; // Multiplies drop chances

    // ---- Adventure. Zone 0 is the safe zone, then ZONES[zone - 1]
    uint8_t zone() const { return zoneNow; }
    bool zoneOpen(uint8_t z) const;
    void setZone(uint8_t z);
    const ZoneDef *zoneDef() const { return zoneNow == 0 ? nullptr : &ZONES[zoneNow - 1]; }
    EnemyState enemyState() const { return enemyStateNow; }
    const EnemyDef *enemy() const;
    bool enemyIsBoss() const { return enemyIndex == ZONE_ENEMIES; }
    double enemyAttack() const;
    double enemyDefense() const;
    double enemyMaxHp() const;
    double enemyHp() const { return enemyHpNow; }
    double respawnLeft() const { return respawnTimer; }
    double advHp() const { return advHpNow; }
    double playerHit() const; // An auto-attack's damage, 0 when the defense blocks it
    double enemyHit() const;
    double strikeReady() const; // 0..1
    bool strike();

    // Monotonic counters for the screens, which only see the game while visible
    uint32_t advKills() const { return advKillCount; }
    uint32_t advDeaths() const { return advDeathCount; }
    uint32_t playerSwings() const { return swingCount; }
    double lastSwing() const { return lastSwingDamage; }
    bool lastSwingStrike() const { return lastSwingWasStrike; }
    uint32_t enemySwings() const { return enemySwingCount; }
    uint32_t drops() const { return dropCount; }
    int8_t lastDrop() const { return lastDropItem; }
    uint32_t bossKills() const { return bossKillCount; }

    // ---- Items: level 0 is not owned
    uint8_t itemLevel(uint8_t item) const { return itemLevels[item]; }
    double itemPercent(uint8_t item, bool attackStat) const;
    int8_t equippedAt(uint8_t place) const { return equipped[place]; }
    static Slot placeSlot(uint8_t place) { return place >= (uint8_t)Slot::ACCESSORY ? Slot::ACCESSORY : (Slot)place; }
    // -1 takes the place's item off. An accessory worn in the other place moves here.
    bool equip(uint8_t place, int8_t item);
    void giveItem(uint8_t item);

    // ---- Gold
    double gold() const { return goldNow; }
    double goldPerSecond() const;
    uint16_t minerCount(uint8_t i) const { return minerCounts[i]; }
    double minerCost(uint8_t i) const;
    bool buyMiner(uint8_t i);
    uint16_t augmentLevel(uint8_t i) const { return augments[i].level; }
    double augmentProgress(uint8_t i) const;
    double augmentGoldCost(uint8_t i) const;
    bool augmentStalled(uint8_t i) const; // Full, waiting for the gold

    // ---- Rebirth
    bool canRebirth() const { return beaten >= REBIRTH_MIN_BOSSES; }
    uint32_t rebirthPoints() const; // What a rebirth now would pay
    uint32_t nan() const { return nanPoints; }
    uint16_t rebirths() const { return rebirthCount; }
    void rebirth();
    uint32_t runSeconds() const { return (uint32_t)runTime; }
    uint32_t playSeconds() const { return (uint32_t)playTime; }

    // ---- Unlocks
    bool isUnlocked(Feature f) const { return (unlocked & (1u << f)) != 0; }
    uint32_t unlockEvents() const { return unlockCount; }
    Feature lastUnlock() const { return lastUnlocked; }
    void unlock(Feature f);

    // Bumped by anything a save should not wait a minute for
    uint32_t milestones() const { return milestoneCount; }

private:
    struct Skill
    {
        uint32_t level;
        double progress;
        double alloc;
    };
    struct Augment
    {
        uint16_t level;
        double progress;
        double alloc;
    };

    uint32_t (*random)();

    double energyNow;
    Skill skills[SKILL_COUNT];
    Augment augments[AUGMENT_COUNT];

    uint16_t beaten;
    uint16_t bestBeaten;
    double bossHpNow;
    double bossPlayerHpNow;
    bool fighting;

    double xpNow;
    uint16_t xpBoughtCount[(uint8_t)XpUpgrade::COUNT];

    uint8_t zoneNow;
    EnemyState enemyStateNow;
    uint8_t enemyIndex;
    double enemyHpNow;
    double respawnTimer;
    double swingTimer;
    double enemySwingTimer;
    double strikeTimer;
    uint8_t killsSinceBoss;
    double advHpNow;
    double lastMaxHp; // A rise in max HP raises both pools with it

    uint8_t itemLevels[ITEM_COUNT];
    int8_t equipped[EQUIP_COUNT];

    double goldNow;
    uint16_t minerCounts[MINER_COUNT];

    uint32_t nanPoints;
    uint16_t rebirthCount;
    double runTime;
    double playTime;

    uint16_t unlocked;
    uint32_t unlockCount;
    Feature lastUnlocked;

    uint32_t advKillCount;
    uint32_t advDeathCount;
    uint32_t swingCount;
    double lastSwingDamage;
    bool lastSwingWasStrike;
    uint32_t enemySwingCount;
    uint32_t dropCount;
    int8_t lastDropItem;
    uint32_t bossKillCount;
    uint32_t milestoneCount;

    void clearRun();
    double &allocSlot(Target t);
    double allocSlotValue(Target t) const;
    uint16_t roll() { return (uint16_t)(random() % 10000); }

    void tickEnergy(double dt);
    void tickTraining(double dt);
    void tickAugments(double dt);
    void tickBoss(double dt);
    void tickAdventure(double dt);
    void checkUnlocks();

    void spawnEnemy();
    void hitEnemy(double damage, bool strike);
    void killEnemy();
    void addGold(double amount);
};

#endif // NAN_GAME_H
