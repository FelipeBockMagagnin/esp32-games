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
#include "managers/SoundManager.h"
#include "screens/InventoryScreen.h"
#include "screens/MiningScreen.h"
#include "screens/CombatScreen.h"
#include "screens/BuildingScreen.h"
#include "screens/UpgradeScreen.h"
#include "screens/ZoneScreen.h"
#include "screens/AchievementScreen.h"

// Screen Dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

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

// Declared before GameState, which reads both for its gold bonuses. Globals in one
// translation unit initialize in declaration order, so the order here matters.
Achievements achievements;
Unlocks unlocks;
Inventory inventory;
GameState game(inventory, achievements);
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

static const uint16_t UNLOCK_ACCENT = 0x07FF; // Cyan, so a new screen never reads as an achievement

// Last achievement count the loop played a sound for
uint8_t seenAchievements = 0;
// Last Unlocks revision the screen locks were applied for
uint8_t seenUnlocks = 0;
// Unlock notice the loop last saw, so its sound plays once when it appears
const char *shownUnlockNotice = nullptr;

// Applies the progression to the rotation: a screen stays out of it until its feature opens
void applyUnlocks()
{
    screenManager.setLocked(&buildingScreen, !unlocks.isUnlocked(Feature::BUILDINGS));
    screenManager.setLocked(&upgradeScreen, !unlocks.isUnlocked(Feature::UPGRADES));
    screenManager.setLocked(&achievementScreen, !unlocks.isUnlocked(Feature::AWARDS));
    screenManager.setLocked(&inventoryScreen, !unlocks.isUnlocked(Feature::INVENTORY));
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
    
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(0x0000);

    // This order is the navigation order. Every screen but Mining starts locked and joins
    // the rotation as the player progresses (see Unlocks); the header labels follow.
    screenManager.addScreen(&miningScreen, "Mining");
    screenManager.addScreen(&buildingScreen, "Buildings", true);
    screenManager.addScreen(&upgradeScreen, "Upgrade", true);
    screenManager.addScreen(&achievementScreen, "Awards", true);
    screenManager.addScreen(&inventoryScreen, "Inventory", true);
    screenManager.addScreen(&zoneScreen, "Zones", true);
    screenManager.addScreen(&combatScreen, "Combat", true);
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

    if (rightButton.wasPressed())
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

    Screen *current = screenManager.getCurrentScreen();
    if (current != nullptr)
    {
        if (unlockNotice != nullptr)
        {
            current->applyNotice("NEW SCREEN - PRESS RIGHT >", unlockNotice, UNLOCK_ACCENT);
        }
        else
        {
            current->applyNotice("ACHIEVEMENT", achievementNotice, GOLD_COLOR);
        }
    }

    screenManager.update(now);
}
