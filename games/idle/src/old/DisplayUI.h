#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <TFT_eSPI.h>
#include "GameEngine.h"

enum ActiveScreen {
    SCREEN_BATTLE,
    SCREEN_SHOP
};

// UI Touch target types
enum TouchAction {
    ACTION_NONE,
    ACTION_SWITCH_TO_SHOP,
    ACTION_SWITCH_TO_BATTLE,
    ACTION_PREV_ZONE,
    ACTION_NEXT_ZONE,
    ACTION_TOUCH_ATTACK,
    ACTION_PREV_SHOP_PAGE,
    ACTION_NEXT_SHOP_PAGE,
    ACTION_BUY_ITEM_0,
    ACTION_BUY_ITEM_1,
    ACTION_BUY_ITEM_2
};

// Color Palette (RGB565)
#define COLOR_BG          0x0862 // Dark obsidian
#define COLOR_PANEL_BG    0x10E5 // Dark slate panel
#define COLOR_PANEL_EDGE  0x29E8 // Slate border
#define COLOR_TEXT_WHITE  0xFFFF
#define COLOR_TEXT_MUTED  0x9CF3
#define COLOR_GOLD        0xFDE0
#define COLOR_CYAN        0x3DFE
#define COLOR_HP_GREEN    0x15D0
#define COLOR_HP_RED      0xF224
#define COLOR_HP_BG       0x3082
#define COLOR_BUTTON_BG   0x19E8
#define COLOR_BUTTON_BUY  0x0BF0
#define COLOR_BUTTON_DIS  0x4A69

class DisplayUI {
public:
    DisplayUI(TFT_eSPI& display, GameEngine& engine);
    void init();
    void setScreen(ActiveScreen screen);
    ActiveScreen getScreen() const { return currentScreen; }
    
    // Rendering passes
    void renderFull();
    void updateDynamic(unsigned long now);
    
    // Touch handling
    TouchAction handleTouch(int touchX, int touchY);

private:
    TFT_eSPI& tft;
    GameEngine& engine;
    ActiveScreen currentScreen;
    
    int shopCurrentPage; // 0 or 1
    
    // Cached values to avoid redundant redraws (zero-flicker)
    int lastPlayerHp;
    int lastPlayerMaxHp;
    int lastEnemyHp;
    int lastEnemyMaxHp;
    uint32_t lastGold;
    int lastZone;
    int lastMonsterIdx;
    bool lastPlayerFlashing;
    bool lastEnemyFlashing;
    char lastCombatMsg[64];
    
    // Render subroutines
    void renderBattleScreenStatic();
    void renderBattleScreenDynamic(unsigned long now);
    
    void renderShopScreenStatic();
    void renderShopScreenDynamic();
    
    // Helper drawing widgets
    void drawHealthBar(int x, int y, int w, int h, int current, int maxVal, bool isEnemy);
    void drawMonsterAvatar(int centerX, int centerY, uint16_t color, bool hitFlash);
    void drawHeroAvatar(int centerX, int centerY, bool hitFlash);
    void drawStatBox(int x, int y, const char* label, const char* value, uint16_t valColor);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t bgColor, uint16_t textColor, uint8_t font);
};

#endif // DISPLAY_UI_H
