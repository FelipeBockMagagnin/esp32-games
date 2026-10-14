#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "pitches.h"
#include "managers/ButtonManager.h"
#include "managers/ScreenManager.h"
#include "game/GameState.h"
#include "game/Inventory.h"
#include "game/Achievements.h"
#include "game/Unlocks.h"
#include "game/CombatState.h"
#include "game/Ascension.h"
#include "game/SaveGame.h"
#include "game/Format.h"
#include "managers/SoundManager.h"
#include "screens/InventoryScreen.h"
#include "screens/MiningScreen.h"
#include "screens/CombatScreen.h"
#include "screens/BuildingScreen.h"
#include "screens/UpgradeScreen.h"
#include "screens/ZoneScreen.h"
#include "screens/AchievementScreen.h"
#include "screens/AscendScreen.h"
#include "screens/ConfirmScreen.h"

// The buttons form a D-pad: Confirm in the middle, Up/Down for in-screen actions,
// Right/Left to step forward/backward through the screens
const int RIGHT_BUTTON_PIN = 23;   // Next screen
const int CONFIRM_BUTTON_PIN = 27; // Confirms on the current screen: mines / buys
const int UP_BUTTON_PIN = 19;      // Navigates up inside the current screen
const int DOWN_BUTTON_PIN = 22;    // Navigates down inside the current screen
const int LEFT_BUTTON_PIN = 26;    // Previous screen, or out of a sub-list
const int BUZZER_PIN = 32;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button upButton = Button(UP_BUTTON_PIN);
Button downButton = Button(DOWN_BUTTON_PIN);
Button leftButton = Button(LEFT_BUTTON_PIN);

// Declared before GameState, which reads them for its bonuses. Globals in one
// translation unit initialize in declaration order, so the order here matters.
Achievements achievements;
Unlocks unlocks;
Inventory inventory;
Ascension ascension;
GameState game(inventory, achievements, ascension);
CombatState combat(game, inventory);
SoundManager sound(BUZZER_PIN);
ScreenManager screenManager(tft);

InventoryScreen inventoryScreen(game, inventory, sound);
MiningScreen miningScreen(game, unlocks, sound);
CombatScreen combatScreen(combat, sound);
BuildingScreen buildingScreen(game, sound);
UpgradeScreen upgradeScreen(game, sound);
ZoneScreen zoneScreen(game, inventory, combat, sound, screenManager);
AchievementScreen achievementScreen(achievements, game, inventory, combat, sound);

// Not in the rotation: modals, the first opened by holding Up and Down (see checkResetGesture)
void resetGame();
void ascendGame();
ConfirmScreen resetScreen({"Reset", "Reset game?", "All progress will be lost,", "ascension and awards too.",
                           "Erase the save and restart", "Resetting..."},
                          Theme::BAD, resetGame, screenManager, sound);
ConfirmScreen ascendConfirm({"Ascend", "Ascend now?", "Gold, buildings and upgrades reset.",
                            "Awards, items and talents stay.", "Restart with the points", "Ascending..."},
                           Theme::EPIC, ascendGame, screenManager, sound, true);
AscendScreen ascendScreen(game, ascension, sound, screenManager, ascendConfirm);

static const uint16_t UNLOCK_ACCENT = Theme::INFO; // Cyan, so a new screen never reads as an achievement

// Last achievement count the loop played a sound for
uint8_t seenAchievements = 0;
// Last Unlocks revision the screen locks were applied for
uint8_t seenUnlocks = 0;
// Unlock notice the loop last saw, so its sound plays once when it appears
const char *shownUnlockNotice = nullptr;

AutoSaver autoSaver(game, inventory, combat, achievements, unlocks, ascension);
// Holds the save read at boot, and the one an ascension writes
SaveBlob saveBuffer;

void resetGame()
{
    eraseSave();
    ESP.restart();
}

// The next run starts from a save rather than by resetting every object in place, so it
// begins exactly like a boot does. A failed write restarts into the old save, which only
// loses the ascension, never the run.
void ascendGame()
{
    if (ascension.ascend(game.getTotalGoldEarned()))
    {
        captureSave(saveBuffer, game, inventory, combat, achievements, unlocks, ascension);
        clearRun(saveBuffer);
        if (!saveGame(saveBuffer))
        {
            Serial.println("Save failed");
        }
    }
    ESP.restart();
}

// Holding Up and Down this long in game asks whether to reset. The first touch of each
// still reaches the screen as an ordinary press; only the long hold opens the question.
static const unsigned long RESET_HOLD_MS = 3000;
unsigned long resetHoldStart = 0;
bool resetHolding = false;
bool resetHoldFired = false; // Latched until a button is released, so one hold opens it once

void checkResetGesture(unsigned long now)
{
    if (!upButton.isDown() || !downButton.isDown())
    {
        resetHolding = false;
        resetHoldFired = false;
        return;
    }
    if (!resetHolding)
    {
        resetHolding = true;
        resetHoldStart = now;
    }
    if (!resetHoldFired && now - resetHoldStart >= RESET_HOLD_MS && !screenManager.hasModal())
    {
        resetHoldFired = true;
        sound.playMenu();
        screenManager.openModal(&resetScreen);
    }
}

// A gold vein can be caught from any screen by holding Up; on Mining a tap does it. Like the
// reset gesture, the first touch still reaches the screen as an ordinary press.
static const unsigned long VEIN_HOLD_MS = 600;
static const unsigned long VEIN_RESULT_MS = 3000;
unsigned long veinHoldStart = 0;
bool veinHolding = false;
bool veinHoldFired = false;
uint32_t seenVeinSpawns = 0;
// What a vein caught off the Mining screen gave, shown in the header
const char *veinResultCaption = nullptr;
char veinResult[24];
unsigned long veinResultUntil = 0;

void checkVeinGesture(unsigned long now)
{
    if (!upButton.isDown() || downButton.isDown())
    {
        veinHolding = false;
        veinHoldFired = false;
        return;
    }
    if (!veinHolding)
    {
        veinHolding = true;
        veinHoldStart = now;
    }
    if (veinHoldFired || now - veinHoldStart < VEIN_HOLD_MS || screenManager.hasModal() ||
        screenManager.getCurrentScreen() == &miningScreen)
    {
        return;
    }
    veinHoldFired = true;

    VeinEffect effect = game.catchVein(now);
    switch (effect)
    {
    case VeinEffect::NONE:
        return;
    case VeinEffect::LUCKY:
        veinResultCaption = "LUCKY!";
        snprintf(veinResult, sizeof(veinResult), "+%s gold", formatAmount(game.getLastLuckyGold()).c_str());
        break;
    case VeinEffect::FRENZY:
        veinResultCaption = "FRENZY!";
        snprintf(veinResult, sizeof(veinResult), "Gold/s x%u %lus", (unsigned)VEIN_FRENZY_MULTIPLIER,
                 (game.getBuffRemaining(now) + 999) / 1000);
        break;
    case VeinEffect::CLICK_FRENZY:
        veinResultCaption = "CLICK FRENZY!";
        snprintf(veinResult, sizeof(veinResult), "Clicks x%u %lus", (unsigned)VEIN_CLICK_MULTIPLIER,
                 (game.getBuffRemaining(now) + 999) / 1000);
        break;
    }
    veinResultUntil = now + VEIN_RESULT_MS;
    sound.playDrop();
}

// Holding Up and Down while the board powers on wipes the save and starts a new game.
// True when a save was loaded into saveBuffer.
bool loadOrReset()
{
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        eraseSave();
        Serial.println("Save erased");
        return false;
    }
    if (loadGame(saveBuffer))
    {
        applySave(saveBuffer, game, inventory, combat, achievements, unlocks, ascension);
        Serial.println("Save loaded");
        return true;
    }
    Serial.println("No save, starting fresh");
    return false;
}

// Applies the progression to the rotation: a screen stays out of it until its feature opens
void applyUnlocks()
{
    screenManager.setLocked(&buildingScreen, !unlocks.isUnlocked(Feature::BUILDINGS));
    screenManager.setLocked(&upgradeScreen, !unlocks.isUnlocked(Feature::UPGRADES));
    screenManager.setLocked(&achievementScreen, !unlocks.isUnlocked(Feature::AWARDS));
    screenManager.setLocked(&inventoryScreen, !unlocks.isUnlocked(Feature::INVENTORY));
    screenManager.setLocked(&ascendScreen, !unlocks.isUnlocked(Feature::ASCEND));
    // Zones and Combat open together: entering a zone jumps straight to the next screen
    screenManager.setLocked(&zoneScreen, !unlocks.isUnlocked(Feature::COMBAT));
    screenManager.setLocked(&combatScreen, !unlocks.isUnlocked(Feature::COMBAT));
}

void setup()
{
    Serial.begin(115200);

    // Drop rolls and enemy picks come from random(); without a seed every boot
    // would roll the same sequence
    randomSeed(esp_random());

    rightButton.setup();
    confirmButton.setup();
    upButton.setup();
    downButton.setup();
    leftButton.setup();
    delay(100);

    // Before the screens and counters below take their first look at the state
    bool loaded = loadOrReset();
    seenAchievements = achievements.getUnlockedCount();
    seenVeinSpawns = game.getVeinSpawns();
    autoSaver.begin(millis(), loaded ? &saveBuffer : nullptr);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::BG);

    // This order is the navigation order. Every screen but Mining starts locked and joins
    // the rotation as the player progresses (see Unlocks); the header labels follow.
    screenManager.addScreen(&miningScreen, "Mining");
    screenManager.addScreen(&buildingScreen, "Buildings", true);
    screenManager.addScreen(&upgradeScreen, "Upgrade", true);
    screenManager.addScreen(&achievementScreen, "Awards", true);
    screenManager.addScreen(&zoneScreen, "Zones", true);
    screenManager.addScreen(&combatScreen, "Combat", true);
    screenManager.addScreen(&inventoryScreen, "Inventory", true);
    screenManager.addScreen(&ascendScreen, "Ascend", true);
    applyUnlocks();
    seenUnlocks = unlocks.getRevision();
}

void loop()
{
    unsigned long now = millis();

    rightButton.loop();
    confirmButton.loop();
    upButton.loop();
    downButton.loop();
    leftButton.loop();

    checkResetGesture(now);
    checkVeinGesture(now);

    if (rightButton.wasPressed() && !screenManager.hasModal())
    {
        sound.playMenu();
        screenManager.nextScreen();
    }

    if (confirmButton.wasPressed())
    {
        screenManager.handleConfirmPress();
    }

    if (upButton.wasPressed())
    {
        screenManager.handleUpPress();
    }

    if (downButton.wasPressed())
    {
        screenManager.handleDownPress();
    }

    if (leftButton.wasPressed())
    {
        if (!screenManager.handleBackPress())
        {
            sound.playMenu();
        }
    }

    // Game progress runs every loop, whichever screen is visible
    game.update(now);
    combat.update(now);

    // Achievements can unlock from anything, so they are checked centrally and the
    // notice is handed to whichever screen is open rather than to a specific one
    achievements.evaluate(now, game, inventory, combat);
    if (achievements.getUnlockedCount() != seenAchievements)
    {
        seenAchievements = achievements.getUnlockedCount();
        sound.playAchievement();
    }

    // Screens open as the player progresses. An unlock's notice waits while an achievement
    // owns the header, since the first achievement is also what opens the Awards screen.
    const char *achievementNotice = achievements.getNotice(now);
    unlocks.evaluate(now, game, inventory, combat, achievements, achievementNotice != nullptr);
    if (unlocks.getRevision() != seenUnlocks)
    {
        seenUnlocks = unlocks.getRevision();
        applyUnlocks();
    }

    const char *unlockNotice = unlocks.getNotice(now);
    if (unlockNotice != nullptr && unlockNotice != shownUnlockNotice)
    {
        sound.playUnlock();
    }
    shownUnlockNotice = unlockNotice;

    if (game.getVeinSpawns() != seenVeinSpawns)
    {
        seenVeinSpawns = game.getVeinSpawns();
        sound.playUnlock();
    }

    Screen *current = screenManager.getCurrentScreen();
    if (current != nullptr)
    {
        if (unlockNotice != nullptr)
        {
            current->applyNotice("NEW SCREEN - PRESS RIGHT >", unlockNotice, UNLOCK_ACCENT);
        }
        else if (achievementNotice != nullptr)
        {
            current->applyNotice("ACHIEVEMENT", achievementNotice, Theme::GOLD);
        }
        else if ((long)(now - veinResultUntil) < 0)
        {
            current->applyNotice(veinResultCaption, veinResult, Theme::GOLD);
        }
        else if (game.isVeinVisible() && current != &miningScreen && !screenManager.hasModal())
        {
            current->applyNotice("HOLD UP TO CATCH IT", "GOLD VEIN!", Theme::GOLD);
        }
        else
        {
            current->applyNotice(nullptr, nullptr, Theme::GOLD);
        }
    }

    screenManager.update(now);

    autoSaver.update(now);
}
