#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <Arduino.h>

#define MAX_ZONES 5
#define MONSTERS_PER_ZONE 4
#define NUM_UPGRADES 6

// Upgrade Types
enum UpgradeType {
    UPGRADE_CLICK_ATK,   // Increases manual click damage
    UPGRADE_PASSIVE_DPS, // Increases passive damage per second
    UPGRADE_ARMOR,       // Increases armor (damage reduction)
    UPGRADE_MAX_HP,      // Increases max HP and HP regen
    UPGRADE_GOLD_BONUS,  // Increases gold rewards (%)
    UPGRADE_AUTO_HEAL    // Stronger passive regeneration
};

struct UpgradeItem {
    const char* name;
    const char* effectDesc;
    UpgradeType type;
    int level;
    uint32_t baseCost;
    float costMultiplier;
    float statPerLevel;
};

struct MonsterTemplate {
    const char* name;
    uint32_t baseHp;
    uint32_t baseAtk;
    uint32_t baseArmor;
    uint32_t baseGold;
    uint16_t spriteColor;
};

struct ZoneConfig {
    const char* name;
    uint16_t themeColor;
    MonsterTemplate monsters[MONSTERS_PER_ZONE];
};

struct PlayerState {
    int maxHp;
    int currentHp;
    int clickAtk;
    int passiveDps;
    int armor;
    int hpRegen;
    float goldBonusMultiplier;
    uint32_t gold;
    int currentZone;
    int highestZoneUnlocked;
    int currentMonsterIndex;
    int monstersDefeatedInZone;
};

struct EnemyState {
    const char* name;
    int maxHp;
    int currentHp;
    int attack;
    int armor;
    uint32_t goldReward;
    uint16_t color;
    unsigned long lastAttackTime;
    unsigned long attackIntervalMs;
};

// Calculate damage with standard RPG armor mitigation formula
inline int calculateDamage(int attack, int targetArmor) {
    if (attack <= 0) return 0;
    float reduction = 100.0f / (100.0f + (float)max(0, targetArmor));
    int dmg = (int)round((float)attack * reduction);
    return max(1, dmg);
}

// Calculate upgrade cost dynamically based on level
inline uint32_t getUpgradeCost(const UpgradeItem& item) {
    float cost = (float)item.baseCost * pow(item.costMultiplier, item.level);
    return (uint32_t)cost;
}

#endif // GAME_DATA_H
