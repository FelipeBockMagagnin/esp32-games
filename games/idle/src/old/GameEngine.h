#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "GameData.h"
#include <Preferences.h>

class GameEngine {
public:
    GameEngine();
    void init();
    void update(unsigned long currentTime);
    
    // Player actions
    bool playerClickAttack();
    bool buyUpgrade(int upgradeIndex);
    bool changeZone(int direction); // -1 = prev, +1 = next
    
    // State Getters
    const PlayerState& getPlayer() const { return player; }
    const EnemyState& getEnemy() const { return enemy; }
    const ZoneConfig& getZone(int idx) const { return zones[idx]; }
    const ZoneConfig& getCurrentZone() const { return zones[player.currentZone]; }
    const UpgradeItem& getUpgrade(int idx) const { return upgrades[idx]; }
    
    // Combat event status
    int getLastDamageDealt() const { return lastDamageDealt; }
    int getLastDamageTaken() const { return lastDamageTaken; }
    bool isEnemyHitFlashing(unsigned long now) const { return now < enemyHitFlashUntil; }
    bool isPlayerHitFlashing(unsigned long now) const { return now < playerHitFlashUntil; }
    bool isPlayerDead() const { return playerDead; }
    int getRespawnSecondsLeft(unsigned long now) const;
    const char* getCombatMessage() const { return combatMessage; }
    uint16_t getCombatMessageColor() const { return combatMsgColor; }
    
    // Save / Load
    void saveProgress();
    void loadProgress();
    void resetProgress();

private:
    PlayerState player;
    EnemyState enemy;
    UpgradeItem upgrades[NUM_UPGRADES];
    ZoneConfig zones[MAX_ZONES];
    Preferences prefs;
    
    // Combat timers and feedback
    unsigned long lastPassiveTickTime;
    unsigned long lastRegenTickTime;
    unsigned long lastAutoSaveTime;
    unsigned long enemyHitFlashUntil;
    unsigned long playerHitFlashUntil;
    unsigned long respawnEndTime;
    
    int lastDamageDealt;
    int lastDamageTaken;
    char combatMessage[64];
    uint16_t combatMsgColor;
    bool playerDead;

    void initZones();
    void initUpgrades();
    void recalculatePlayerStats();
    void spawnMonster(int zoneIdx, int monsterIdx);
    void handleEnemyDeath();
    void handlePlayerDeath();
    void setCombatMessage(const char* msg, uint16_t color);
};

#endif // GAME_ENGINE_H
