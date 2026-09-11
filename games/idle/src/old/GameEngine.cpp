#ifndef TFT_RED
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF
#define TFT_CYAN 0x07FF
#define TFT_ORANGE 0xFDA0
#endif

#include "GameEngine.h"

GameEngine::GameEngine() {
    lastPassiveTickTime = 0;
    lastRegenTickTime = 0;
    lastAutoSaveTime = 0;
    enemyHitFlashUntil = 0;
    playerHitFlashUntil = 0;
    respawnEndTime = 0;
    lastDamageDealt = 0;
    lastDamageTaken = 0;
    playerDead = false;
    combatMsgColor = TFT_WHITE;
    combatMessage[0] = '\0';
}

void GameEngine::init() {
    initZones();
    initUpgrades();
    
    player.maxHp = 100;
    player.currentHp = 100;
    player.clickAtk = 5;
    player.passiveDps = 0;
    player.armor = 2;
    player.hpRegen = 1;
    player.goldBonusMultiplier = 1.0f;
    player.gold = 0;
    player.currentZone = 0;
    player.highestZoneUnlocked = 0;
    player.currentMonsterIndex = 0;
    player.monstersDefeatedInZone = 0;
    
    loadProgress();
    recalculatePlayerStats();
    spawnMonster(player.currentZone, player.currentMonsterIndex);
    
    setCombatMessage("Ready! Click D4 or Tap Screen to Attack", TFT_CYAN);
}

void GameEngine::initZones() {
    // Zone 0: Whispering Woods
    zones[0].name = "Whispering Woods";
    zones[0].themeColor = 0x2E65; // Lush Forest Green
    zones[0].monsters[0] = {"Green Slime", 30, 4, 1, 5, 0x07E0};
    zones[0].monsters[1] = {"Goblin Scout", 55, 7, 2, 10, 0x7E08};
    zones[0].monsters[2] = {"Dire Wolf", 90, 10, 3, 16, 0xCE79};
    zones[0].monsters[3] = {"Forest Treant", 140, 14, 5, 28, 0x2444};

    // Zone 1: Sunken Crypt
    zones[1].name = "Sunken Crypt";
    zones[1].themeColor = 0x5ADF; // Crypt Cyan/Slate
    zones[1].monsters[0] = {"Skeleton Grunt", 200, 18, 7, 45, 0xDEFB};
    zones[1].monsters[1] = {"Tomb Wraith", 280, 24, 10, 70, 0x9D7F};
    zones[1].monsters[2] = {"Crypt Ghoul", 380, 30, 14, 105, 0x6338};
    zones[1].monsters[3] = {"Bone Colossus", 520, 38, 18, 160, 0xB596};

    // Zone 2: Scorched Peak
    zones[2].name = "Scorched Peak";
    zones[2].themeColor = 0xFA08; // Fiery Crimson
    zones[2].monsters[0] = {"Magma Imp", 720, 48, 22, 230, 0xFDA0};
    zones[2].monsters[1] = {"Lava Hound", 980, 58, 28, 340, 0xF800};
    zones[2].monsters[2] = {"Fire Elemental", 1350, 70, 35, 480, 0xFE40};
    zones[2].monsters[3] = {"Flame Lord", 1850, 85, 45, 680, 0xC800};

    // Zone 3: Shadow Realm
    zones[3].name = "Shadow Realm";
    zones[3].themeColor = 0x917C; // Shadow Purple
    zones[3].monsters[0] = {"Shadow Stalker", 2500, 105, 55, 950, 0x81F0};
    zones[3].monsters[1] = {"Nether Phantom", 3300, 125, 68, 1350, 0xA11F};
    zones[3].monsters[2] = {"Lich Knight", 4300, 150, 80, 1850, 0x780F};
    zones[3].monsters[3] = {"Void Behemoth", 5600, 180, 100, 2600, 0x4810};

    // Zone 4: Dragon Citadel
    zones[4].name = "Dragon Citadel";
    zones[4].themeColor = 0xFDE0; // Golden Dragon
    zones[4].monsters[0] = {"Storm Wyvern", 7200, 220, 120, 3500, 0x05BF};
    zones[4].monsters[1] = {"Chaos Drake", 9300, 260, 145, 4800, 0xFA20};
    zones[4].monsters[2] = {"Elder Dragon", 12500, 310, 175, 6500, 0xF800};
    zones[4].monsters[3] = {"Dragon Deity", 16500, 390, 220, 9500, 0xFFE0};
}

void GameEngine::initUpgrades() {
    upgrades[0] = {"Iron Blade", "+3 Click ATK", UPGRADE_CLICK_ATK, 0, 10, 1.32f, 3.0f};
    upgrades[1] = {"Companion", "+2 Passive DPS", UPGRADE_PASSIVE_DPS, 0, 25, 1.28f, 2.0f};
    upgrades[2] = {"Plate Mail", "+2 Armor", UPGRADE_ARMOR, 0, 20, 1.25f, 2.0f};
    upgrades[3] = {"Vitality Gem", "+15 Max HP", UPGRADE_MAX_HP, 0, 15, 1.22f, 15.0f};
    upgrades[4] = {"Midas Amulet", "+10% Gold", UPGRADE_GOLD_BONUS, 0, 30, 1.35f, 0.10f};
    upgrades[5] = {"Holy Chalice", "+1 HP Regen/s", UPGRADE_AUTO_HEAL, 0, 40, 1.30f, 1.0f};
}

void GameEngine::recalculatePlayerStats() {
    int oldMaxHp = player.maxHp;
    
    player.clickAtk = 5 + (int)(upgrades[0].level * upgrades[0].statPerLevel);
    player.passiveDps = 0 + (int)(upgrades[1].level * upgrades[1].statPerLevel);
    player.armor = 2 + (int)(upgrades[2].level * upgrades[2].statPerLevel);
    player.maxHp = 100 + (int)(upgrades[3].level * upgrades[3].statPerLevel);
    player.goldBonusMultiplier = 1.0f + (upgrades[4].level * upgrades[4].statPerLevel);
    player.hpRegen = 1 + (int)(upgrades[5].level * upgrades[5].statPerLevel);
    
    // Scale current HP if max HP increased
    if (player.maxHp > oldMaxHp && !playerDead) {
        player.currentHp += (player.maxHp - oldMaxHp);
    }
    if (player.currentHp > player.maxHp) {
        player.currentHp = player.maxHp;
    }
}

void GameEngine::spawnMonster(int zoneIdx, int monsterIdx) {
    if (zoneIdx < 0) zoneIdx = 0;
    if (zoneIdx >= MAX_ZONES) zoneIdx = MAX_ZONES - 1;
    if (monsterIdx < 0) monsterIdx = 0;
    if (monsterIdx >= MONSTERS_PER_ZONE) monsterIdx = MONSTERS_PER_ZONE - 1;
    
    const MonsterTemplate& tmpl = zones[zoneIdx].monsters[monsterIdx];
    enemy.name = tmpl.name;
    enemy.maxHp = tmpl.baseHp;
    enemy.currentHp = tmpl.baseHp;
    enemy.attack = tmpl.baseAtk;
    enemy.armor = tmpl.baseArmor;
    enemy.goldReward = tmpl.baseGold;
    enemy.color = tmpl.spriteColor;
    enemy.lastAttackTime = millis();
    enemy.attackIntervalMs = 1800; // Attack every 1.8 seconds
}

bool GameEngine::playerClickAttack() {
    if (playerDead) return false;
    
    int dmg = calculateDamage(player.clickAtk, enemy.armor);
    enemy.currentHp -= dmg;
    lastDamageDealt = dmg;
    enemyHitFlashUntil = millis() + 160;
    
    char buf[48];
    snprintf(buf, sizeof(buf), "Hit %s for %d DMG!", enemy.name, dmg);
    setCombatMessage(buf, TFT_YELLOW);
    
    if (enemy.currentHp <= 0) {
        handleEnemyDeath();
    }
    return true;
}

void GameEngine::handleEnemyDeath() {
    uint32_t goldEarned = (uint32_t)round((float)enemy.goldReward * player.goldBonusMultiplier);
    player.gold += goldEarned;
    player.monstersDefeatedInZone++;
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Defeated %s! +%u Gold", enemy.name, goldEarned);
    setCombatMessage(buf, TFT_GREEN);
    
    // Check zone unlock: unlock next zone if defeated enough monsters in this zone
    if (player.currentZone == player.highestZoneUnlocked && 
        player.highestZoneUnlocked < MAX_ZONES - 1 && 
        player.monstersDefeatedInZone >= 5) {
        player.highestZoneUnlocked++;
        snprintf(buf, sizeof(buf), "NEW ZONE UNLOCKED: %s!", zones[player.highestZoneUnlocked].name);
        setCombatMessage(buf, TFT_CYAN);
    }
    
    // Advance to next monster or loop
    player.currentMonsterIndex = (player.currentMonsterIndex + 1) % MONSTERS_PER_ZONE;
    spawnMonster(player.currentZone, player.currentMonsterIndex);
}

void GameEngine::handlePlayerDeath() {
    playerDead = true;
    player.currentHp = 0;
    respawnEndTime = millis() + 3000; // 3 seconds respawn time
    setCombatMessage("You were defeated! Respawning in 3s...", TFT_RED);
}

bool GameEngine::changeZone(int direction) {
    int targetZone = player.currentZone + direction;
    if (targetZone < 0 || targetZone > player.highestZoneUnlocked) {
        return false;
    }
    player.currentZone = targetZone;
    player.currentMonsterIndex = 0;
    player.monstersDefeatedInZone = 0;
    spawnMonster(player.currentZone, player.currentMonsterIndex);
    
    char buf[48];
    snprintf(buf, sizeof(buf), "Entering %s...", zones[player.currentZone].name);
    setCombatMessage(buf, zones[player.currentZone].themeColor);
    return true;
}

bool GameEngine::buyUpgrade(int upgradeIndex) {
    if (upgradeIndex < 0 || upgradeIndex >= NUM_UPGRADES) return false;
    
    UpgradeItem& item = upgrades[upgradeIndex];
    uint32_t cost = getUpgradeCost(item);
    
    if (player.gold >= cost) {
        player.gold -= cost;
        item.level++;
        recalculatePlayerStats();
        
        char buf[48];
        snprintf(buf, sizeof(buf), "Upgraded %s to Lv.%d!", item.name, item.level);
        setCombatMessage(buf, TFT_GREEN);
        return true;
    } else {
        setCombatMessage("Not enough Gold!", TFT_RED);
        return false;
    }
}

void GameEngine::update(unsigned long currentTime) {
    // Handle respawn countdown
    if (playerDead) {
        if (currentTime >= respawnEndTime) {
            playerDead = false;
            player.currentHp = player.maxHp;
            setCombatMessage("Revived at full health!", TFT_CYAN);
        }
        return; // Don't do combat while dead
    }
    
    // 1. Passive DPS Tick (Every 1000ms)
    if (currentTime - lastPassiveTickTime >= 1000) {
        lastPassiveTickTime = currentTime;
        if (player.passiveDps > 0) {
            int pDmg = calculateDamage(player.passiveDps, enemy.armor);
            enemy.currentHp -= pDmg;
            lastDamageDealt = pDmg;
            enemyHitFlashUntil = currentTime + 100;
            
            if (enemy.currentHp <= 0) {
                handleEnemyDeath();
            }
        }
    }
    
    // 2. HP Regen Tick (Every 1000ms)
    if (currentTime - lastRegenTickTime >= 1000) {
        lastRegenTickTime = currentTime;
        if (player.currentHp < player.maxHp) {
            player.currentHp = min(player.maxHp, player.currentHp + player.hpRegen);
        }
    }
    
    // 3. Enemy Attack Timer
    if (currentTime - enemy.lastAttackTime >= enemy.attackIntervalMs) {
        enemy.lastAttackTime = currentTime;
        int eDmg = calculateDamage(enemy.attack, player.armor);
        player.currentHp -= eDmg;
        lastDamageTaken = eDmg;
        playerHitFlashUntil = currentTime + 180;
        
        char buf[48];
        snprintf(buf, sizeof(buf), "%s attacked for %d DMG!", enemy.name, eDmg);
        setCombatMessage(buf, TFT_RED);
        
        if (player.currentHp <= 0) {
            handlePlayerDeath();
        }
    }
    
    // 4. Auto-save every 30 seconds
    if (currentTime - lastAutoSaveTime >= 30000) {
        lastAutoSaveTime = currentTime;
        saveProgress();
    }
}

int GameEngine::getRespawnSecondsLeft(unsigned long now) const {
    if (!playerDead || now >= respawnEndTime) return 0;
    return (int)((respawnEndTime - now + 999) / 1000);
}

void GameEngine::setCombatMessage(const char* msg, uint16_t color) {
    strncpy(combatMessage, msg, sizeof(combatMessage) - 1);
    combatMessage[sizeof(combatMessage) - 1] = '\0';
    combatMsgColor = color;
}

void GameEngine::saveProgress() {
    prefs.begin("idlegame", false);
    prefs.putUInt("gold", player.gold);
    prefs.putInt("curZone", player.currentZone);
    prefs.putInt("maxZone", player.highestZoneUnlocked);
    
    for (int i = 0; i < NUM_UPGRADES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "upg_%d", i);
        prefs.putInt(key, upgrades[i].level);
    }
    prefs.end();
}

void GameEngine::loadProgress() {
    prefs.begin("idlegame", true);
    player.gold = prefs.getUInt("gold", 0);
    player.currentZone = prefs.getInt("curZone", 0);
    player.highestZoneUnlocked = prefs.getInt("maxZone", 0);
    
    for (int i = 0; i < NUM_UPGRADES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "upg_%d", i);
        upgrades[i].level = prefs.getInt(key, 0);
    }
    prefs.end();
    
    // Bounds check loaded zones
    if (player.highestZoneUnlocked < 0 || player.highestZoneUnlocked >= MAX_ZONES) {
        player.highestZoneUnlocked = 0;
    }
    if (player.currentZone < 0 || player.currentZone > player.highestZoneUnlocked) {
        player.currentZone = 0;
    }
}

void GameEngine::resetProgress() {
    prefs.begin("idlegame", false);
    prefs.clear();
    prefs.end();
    init();
}
