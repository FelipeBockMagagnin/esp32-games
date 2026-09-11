#include "DisplayUI.h"

DisplayUI::DisplayUI(TFT_eSPI& display, GameEngine& eng) 
    : tft(display), engine(eng) {
    currentScreen = SCREEN_BATTLE;
    shopCurrentPage = 0;
    
    lastPlayerHp = -1;
    lastPlayerMaxHp = -1;
    lastEnemyHp = -1;
    lastEnemyMaxHp = -1;
    lastGold = 0xFFFFFFFF;
    lastZone = -1;
    lastMonsterIdx = -1;
    lastPlayerFlashing = false;
    lastEnemyFlashing = false;
    lastCombatMsg[0] = '\0';
}

void DisplayUI::init() {
    renderFull();
}

void DisplayUI::setScreen(ActiveScreen screen) {
    currentScreen = screen;
    // Reset cache so screen redraws cleanly
    lastPlayerHp = -1;
    lastEnemyHp = -1;
    lastGold = 0xFFFFFFFF;
    lastZone = -1;
    lastMonsterIdx = -1;
    lastCombatMsg[0] = '\0';
    renderFull();
}

void DisplayUI::renderFull() {
    tft.fillScreen(COLOR_BG);
    if (currentScreen == SCREEN_BATTLE) {
        renderBattleScreenStatic();
        renderBattleScreenDynamic(millis());
    } else {
        renderShopScreenStatic();
        renderShopScreenDynamic();
    }
}

void DisplayUI::updateDynamic(unsigned long now) {
    if (currentScreen == SCREEN_BATTLE) {
        renderBattleScreenDynamic(now);
    } else {
        renderShopScreenDynamic();
    }
}


// -------------------------------------------------------------
// BATTLE SCREEN RENDERING
// -------------------------------------------------------------

void DisplayUI::renderBattleScreenStatic() {
    // 1. Top Bar Frame
    tft.drawFastHLine(0, 26, 320, COLOR_PANEL_EDGE);
    
    // Zone Navigation Buttons
    drawButton(2, 2, 24, 22, "<", COLOR_BUTTON_BG, COLOR_TEXT_WHITE, 1);
    drawButton(180, 2, 24, 22, ">", COLOR_BUTTON_BG, COLOR_TEXT_WHITE, 1);
    
    // 2. Player Panel (Left)
    tft.fillRoundRect(4, 29, 153, 154, 4, COLOR_PANEL_BG);
    tft.drawRoundRect(4, 29, 153, 154, 4, COLOR_PANEL_EDGE);
    tft.setTextColor(COLOR_CYAN, COLOR_PANEL_BG);
    tft.drawCentreString("HERO", 80, 32, 2);
    
    // Static Labels for Player Stats
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL_BG);
    tft.drawString("ATK:", 12, 116, 1);
    tft.drawString("ARM:", 12, 130, 1);
    tft.drawString("DPS:", 12, 144, 1);
    tft.drawString("REG:", 12, 158, 1);
    
    // 3. Enemy Panel (Right)
    tft.fillRoundRect(163, 29, 153, 154, 4, COLOR_PANEL_BG);
    tft.drawRoundRect(163, 29, 153, 154, 4, COLOR_PANEL_EDGE);
    
    // Static Labels for Enemy Stats
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL_BG);
    tft.drawString("ATK:", 171, 116, 1);
    tft.drawString("ARM:", 171, 130, 1);
    tft.drawString("Gold:", 171, 144, 1);
    
    // 4. Combat Message Box Frame
    tft.fillRect(4, 186, 312, 17, 0x0000);
    tft.drawRect(4, 186, 312, 17, COLOR_PANEL_EDGE);
    
    // 5. Bottom Navigation Bar Button
    tft.fillRoundRect(4, 206, 312, 30, 4, 0x1A4F);
    tft.drawRoundRect(4, 206, 312, 30, 4, COLOR_CYAN);
    tft.setTextColor(COLOR_TEXT_WHITE, 0x1A4F);
    tft.drawCentreString("SHOP & UPGRADES  >>", 160, 212, 2);
}

void DisplayUI::renderBattleScreenDynamic(unsigned long now) {
    const PlayerState& player = engine.getPlayer();
    const EnemyState& enemy = engine.getEnemy();
    const ZoneConfig& zone = engine.getCurrentZone();
    
    bool pFlash = engine.isPlayerHitFlashing(now);
    bool eFlash = engine.isEnemyHitFlashing(now);
    
    // 1. Zone Name Header (Update if zone changed)
    if (lastZone != player.currentZone) {
        lastZone = player.currentZone;
        tft.fillRect(28, 2, 150, 22, COLOR_BG);
        tft.setTextColor(zone.themeColor, COLOR_BG);
        char zoneStr[32];
        snprintf(zoneStr, sizeof(zoneStr), "Z%d: %s", player.currentZone + 1, zone.name);
        tft.drawCentreString(zoneStr, 103, 5, 1);
        
        // Update Zone navigation button colors (dim if can't move)
        uint16_t prevCol = (player.currentZone > 0) ? COLOR_BUTTON_BG : COLOR_BUTTON_DIS;
        uint16_t nextCol = (player.currentZone < player.highestZoneUnlocked) ? COLOR_BUTTON_BG : COLOR_BUTTON_DIS;
        drawButton(2, 2, 24, 22, "<", prevCol, COLOR_TEXT_WHITE, 1);
        drawButton(180, 2, 24, 22, ">", nextCol, COLOR_TEXT_WHITE, 1);
    }
    
    // 2. Gold Counter (Update if changed)
    if (lastGold != player.gold) {
        lastGold = player.gold;
        tft.fillRect(206, 2, 110, 22, COLOR_BG);
        tft.setTextColor(COLOR_GOLD, COLOR_BG);
        char goldStr[24];
        snprintf(goldStr, sizeof(goldStr), "%u G", player.gold);
        tft.drawRightString(goldStr, 314, 5, 2);
    }
    
    // 3. Player Stats & Avatar
    if (lastPlayerFlashing != pFlash || lastPlayerHp != player.currentHp || lastPlayerMaxHp != player.maxHp) {
        lastPlayerFlashing = pFlash;
        lastPlayerHp = player.currentHp;
        lastPlayerMaxHp = player.maxHp;
        
        // Avatar
        drawHeroAvatar(80, 68, pFlash);
        
        // HP Bar
        drawHealthBar(10, 94, 141, 14, player.currentHp, player.maxHp, false);
        
        // Player stats values
        tft.setTextColor(COLOR_TEXT_WHITE, COLOR_PANEL_BG);
        char buf[20];
        
        // Attack
        snprintf(buf, sizeof(buf), "%-6d", player.clickAtk);
        tft.drawString(buf, 44, 116, 1);
        
        // Armor
        snprintf(buf, sizeof(buf), "%-6d", player.armor);
        tft.drawString(buf, 44, 130, 1);
        
        // Passive DPS
        snprintf(buf, sizeof(buf), "%d/s   ", player.passiveDps);
        tft.drawString(buf, 44, 144, 1);
        
        // Regen
        snprintf(buf, sizeof(buf), "%d/s   ", player.hpRegen);
        tft.drawString(buf, 44, 158, 1);
    }
    
    // 4. Enemy Stats & Avatar
    if (lastMonsterIdx != player.currentMonsterIndex) {
        lastMonsterIdx = player.currentMonsterIndex;
        // Monster name banner
        tft.fillRect(166, 31, 147, 18, COLOR_PANEL_BG);
        tft.setTextColor(enemy.color, COLOR_PANEL_BG);
        tft.drawCentreString(enemy.name, 239, 32, 2);
        
        // Enemy stats
        tft.setTextColor(COLOR_TEXT_WHITE, COLOR_PANEL_BG);
        char buf[20];
        snprintf(buf, sizeof(buf), "%-6d", enemy.attack);
        tft.drawString(buf, 206, 116, 1);
        snprintf(buf, sizeof(buf), "%-6d", enemy.armor);
        tft.drawString(buf, 206, 130, 1);
        snprintf(buf, sizeof(buf), "+%-5u", enemy.goldReward);
        tft.drawString(buf, 206, 144, 1);
    }
    
    if (lastEnemyFlashing != eFlash || lastEnemyHp != enemy.currentHp || lastEnemyMaxHp != enemy.maxHp) {
        lastEnemyFlashing = eFlash;
        lastEnemyHp = enemy.currentHp;
        lastEnemyMaxHp = enemy.maxHp;
        
        // Avatar
        drawMonsterAvatar(240, 68, enemy.color, eFlash);
        
        // HP Bar
        drawHealthBar(169, 94, 141, 14, enemy.currentHp, enemy.maxHp, true);
    }
    
    // 5. Combat Message Ticker
    const char* curMsg = engine.getCombatMessage();
    if (strcmp(lastCombatMsg, curMsg) != 0) {
        strncpy(lastCombatMsg, curMsg, sizeof(lastCombatMsg) - 1);
        lastCombatMsg[sizeof(lastCombatMsg) - 1] = '\0';
        
        tft.fillRect(6, 188, 308, 13, 0x0000);
        tft.setTextColor(engine.getCombatMessageColor(), 0x0000);
        tft.drawCentreString(curMsg, 160, 189, 1);
    }
}

// -------------------------------------------------------------
// SHOP SCREEN RENDERING
// -------------------------------------------------------------

void DisplayUI::renderShopScreenStatic() {
    // 1. Top Bar
    drawButton(4, 2, 85, 22, "< BATTLE", COLOR_BUTTON_BG, COLOR_CYAN, 1);
    tft.setTextColor(COLOR_TEXT_WHITE, COLOR_BG);
    tft.drawCentreString("UPGRADE SHOP", 160, 5, 2);
    tft.drawFastHLine(0, 26, 320, COLOR_PANEL_EDGE);
    
    // 2. Bottom Navigation
    drawButton(10, 206, 85, 28, "< PREV", COLOR_BUTTON_BG, COLOR_TEXT_WHITE, 1);
    drawButton(225, 206, 85, 28, "NEXT >", COLOR_BUTTON_BG, COLOR_TEXT_WHITE, 1);
    
    char pageBuf[16];
    snprintf(pageBuf, sizeof(pageBuf), "Page %d / 2", shopCurrentPage + 1);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    tft.drawCentreString(pageBuf, 160, 212, 1);
}

void DisplayUI::renderShopScreenDynamic() {
    const PlayerState& player = engine.getPlayer();
    
    // 1. Update Gold Counter
    if (lastGold != player.gold) {
        lastGold = player.gold;
        tft.fillRect(215, 2, 102, 22, COLOR_BG);
        tft.setTextColor(COLOR_GOLD, COLOR_BG);
        char goldStr[20];
        snprintf(goldStr, sizeof(goldStr), "%u G", player.gold);
        tft.drawRightString(goldStr, 314, 5, 2);
    }
    
    // 2. Render 3 Upgrade Cards for the Current Page
    int startIdx = shopCurrentPage * 3;
    for (int i = 0; i < 3; i++) {
        int upgIdx = startIdx + i;
        if (upgIdx >= NUM_UPGRADES) break;
        
        const UpgradeItem& item = engine.getUpgrade(upgIdx);
        uint32_t cost = getUpgradeCost(item);
        bool canAfford = (player.gold >= cost);
        
        int rowY = 30 + (i * 58);
        
        // Card Background
        tft.fillRoundRect(4, rowY, 312, 54, 4, COLOR_PANEL_BG);
        tft.drawRoundRect(4, rowY, 312, 54, 4, COLOR_PANEL_EDGE);
        
        // Item Name and Level
        tft.setTextColor(COLOR_TEXT_WHITE, COLOR_PANEL_BG);
        char nameBuf[32];
        snprintf(nameBuf, sizeof(nameBuf), "%s (Lv.%d)", item.name, item.level);
        tft.drawString(nameBuf, 14, rowY + 8, 2);
        
        // Item Effect Description
        tft.setTextColor(COLOR_CYAN, COLOR_PANEL_BG);
        tft.drawString(item.effectDesc, 14, rowY + 30, 1);
        
        // Buy Button
        uint16_t btnBg = canAfford ? COLOR_BUTTON_BUY : COLOR_BUTTON_DIS;
        uint16_t textCol = canAfford ? 0x0000 : COLOR_TEXT_MUTED;
        
        tft.fillRoundRect(230, rowY + 6, 80, 42, 4, btnBg);
        tft.drawRoundRect(230, rowY + 6, 80, 42, 4, canAfford ? 0xFFFF : COLOR_PANEL_EDGE);
        
        tft.setTextColor(textCol, btnBg);
        tft.drawCentreString("BUY", 270, rowY + 10, 1);
        
        char costBuf[16];
        snprintf(costBuf, sizeof(costBuf), "%u G", cost);
        tft.drawCentreString(costBuf, 270, rowY + 26, 1);
    }
}

// -------------------------------------------------------------
// DRAWING HELPERS
// -------------------------------------------------------------

void DisplayUI::drawHealthBar(int x, int y, int w, int h, int current, int maxVal, bool isEnemy) {
    if (maxVal <= 0) maxVal = 1;
    if (current < 0) current = 0;
    if (current > maxVal) current = maxVal;
    
    int fillW = (current * (w - 2)) / maxVal;
    uint16_t barColor = isEnemy ? COLOR_HP_RED : COLOR_HP_GREEN;
    
    // Background and border
    tft.drawRect(x, y, w, h, COLOR_PANEL_EDGE);
    tft.fillRect(x + 1, y + 1, fillW, h - 2, barColor);
    tft.fillRect(x + 1 + fillW, y + 1, (w - 2) - fillW, h - 2, COLOR_HP_BG);
    
    // Numeric HP overlay
    char hpText[24];
    snprintf(hpText, sizeof(hpText), "%d/%d", current, maxVal);
    tft.setTextColor(COLOR_TEXT_WHITE, barColor);
    tft.drawCentreString(hpText, x + (w / 2), y + 2, 1);
}

void DisplayUI::drawHeroAvatar(int centerX, int centerY, bool hitFlash) {
    uint16_t helmetColor = hitFlash ? 0xF800 : 0x7BEF;
    uint16_t visorColor = hitFlash ? 0xFFFF : 0x07FF;
    
    // Clear area around avatar
    tft.fillRect(centerX - 24, centerY - 20, 48, 40, COLOR_PANEL_BG);
    
    // Knight Helmet outline
    tft.fillCircle(centerX, centerY - 2, 16, helmetColor);
    tft.fillRect(centerX - 12, centerY - 2, 24, 16, helmetColor);
    
    // Visor slit
    tft.fillRect(centerX - 10, centerY - 2, 20, 4, visorColor);
    
    // Helmet crest
    tft.fillTriangle(centerX - 4, centerY - 14, centerX + 4, centerY - 14, centerX, centerY - 20, 0xFDA0);
}

void DisplayUI::drawMonsterAvatar(int centerX, int centerY, uint16_t color, bool hitFlash) {
    uint16_t drawColor = hitFlash ? 0xFFFF : color;
    uint16_t eyeColor = hitFlash ? 0xF800 : 0xFFFF;
    
    // Clear area around monster
    tft.fillRect(centerX - 24, centerY - 20, 48, 40, COLOR_PANEL_BG);
    
    // Monster Body: Rounded creature silhouette
    tft.fillRoundRect(centerX - 18, centerY - 10, 36, 26, 8, drawColor);
    
    // Horns / Ears
    tft.fillTriangle(centerX - 14, centerY - 8, centerX - 8, centerY - 8, centerX - 16, centerY - 18, drawColor);
    tft.fillTriangle(centerX + 14, centerY - 8, centerX + 8, centerY - 8, centerX + 16, centerY - 18, drawColor);
    
    // Glowing Eyes
    tft.fillCircle(centerX - 7, centerY + 1, 3, eyeColor);
    tft.fillCircle(centerX + 7, centerY + 1, 3, eyeColor);
    tft.fillCircle(centerX - 7, centerY + 1, 1, 0x0000);
    tft.fillCircle(centerX + 7, centerY + 1, 1, 0x0000);
}

void DisplayUI::drawButton(int x, int y, int w, int h, const char* label, uint16_t bgColor, uint16_t textColor, uint8_t font) {
    tft.fillRoundRect(x, y, w, h, 3, bgColor);
    tft.drawRoundRect(x, y, w, h, 3, COLOR_PANEL_EDGE);
    tft.setTextColor(textColor, bgColor);
    tft.drawCentreString(label, x + (w / 2), y + (h / 2) - 4, font);
}

// -------------------------------------------------------------
// TOUCH HIT TESTING
// -------------------------------------------------------------

TouchAction DisplayUI::handleTouch(int touchX, int touchY) {
    if (currentScreen == SCREEN_BATTLE) {
        // Prev Zone Button (x: 0..40, y: 0..28)
        if (touchX >= 0 && touchX <= 40 && touchY >= 0 && touchY <= 28) {
            return ACTION_PREV_ZONE;
        }
        // Next Zone Button (x: 170..215, y: 0..28)
        if (touchX >= 170 && touchX <= 215 && touchY >= 0 && touchY <= 28) {
            return ACTION_NEXT_ZONE;
        }
        // Shop Button at bottom (y: 195..240)
        if (touchY >= 195) {
            return ACTION_SWITCH_TO_SHOP;
        }
        // Tapping anywhere in the battle arena area (Hero or Monster) triggers attack
        if (touchY >= 30 && touchY < 195) {
            return ACTION_TOUCH_ATTACK;
        }
    } else if (currentScreen == SCREEN_SHOP) {
        // Back to Battle Button (x: 0..120, y: 0..35)
        if (touchX >= 0 && touchX <= 120 && touchY >= 0 && touchY <= 35) {
            return ACTION_SWITCH_TO_BATTLE;
        }
        // Prev Page Button (x: 0..120, y: 195..240)
        if (touchX >= 0 && touchX <= 120 && touchY >= 195) {
            if (shopCurrentPage > 0) {
                shopCurrentPage--;
                renderFull();
            }
            return ACTION_PREV_SHOP_PAGE;
        }
        // Next Page Button (x: 200..320, y: 195..240)
        if (touchX >= 200 && touchX <= 320 && touchY >= 195) {
            if (shopCurrentPage < 1) {
                shopCurrentPage++;
                renderFull();
            }
            return ACTION_NEXT_SHOP_PAGE;
        }
        // Tap anywhere on Card 0 (y: 30..84)
        if (touchY >= 30 && touchY <= 84) {
            return (shopCurrentPage == 0) ? ACTION_BUY_ITEM_0 : (TouchAction)(ACTION_BUY_ITEM_0 + 3);
        }
        // Tap anywhere on Card 1 (y: 85..139)
        if (touchY >= 85 && touchY <= 139) {
            return (shopCurrentPage == 0) ? ACTION_BUY_ITEM_1 : (TouchAction)(ACTION_BUY_ITEM_1 + 3);
        }
        // Tap anywhere on Card 2 (y: 140..194)
        if (touchY >= 140 && touchY <= 194) {
            return (shopCurrentPage == 0) ? ACTION_BUY_ITEM_2 : (TouchAction)(ACTION_BUY_ITEM_2 + 3);
        }
    }
    
    return ACTION_NONE;
}
