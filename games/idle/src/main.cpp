#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "pitches.h"
#include "managers/ButtonManager.h"
#include "managers/ScreenManager.h"
#include "game/GameState.h"
#include "game/Inventory.h"
#include "game/CombatState.h"
#include "managers/SoundManager.h"
#include "managers/ClimateManager.h"
#include "managers/LuminosityManager.h"
#include "screens/InventoryScreen.h"
#include "screens/MiningScreen.h"
#include "screens/CombatScreen.h"
#include "screens/BuildingScreen.h"
#include "screens/UpgradeScreen.h"
#include "screens/ZoneScreen.h"

// Screen Dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

const int MENU_BUTTON_PIN = 23;    // Switches to the next screen
const int CONFIRM_BUTTON_PIN = 27; // Confirms on the current screen: mines / buys
const int SELECT_BUTTON_PIN = 22;  // Selects the next item inside the current screen
const int BUZZER_PIN = 32;
const int DHT_PIN = 25;        // Must be output-capable, so not 34-39
const int LUMINOSITY_PIN = 34; // LDR, read through ADC1

TFT_eSPI tft = TFT_eSPI();
Button menuButton = Button(MENU_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button selectButton = Button(SELECT_BUTTON_PIN);

// Declared before GameState: it reads the inventory for item gold bonuses, and
// globals in one translation unit initialize in declaration order
Inventory inventory;
GameState game(inventory);
CombatState combat(game, inventory);
SoundManager sound(BUZZER_PIN);
ClimateManager climate(DHT_PIN);
LuminosityManager luminosity(LUMINOSITY_PIN);
ScreenManager screenManager(tft);

InventoryScreen inventoryScreen(game, inventory, sound);
MiningScreen miningScreen(game, sound, climate, luminosity);
CombatScreen combatScreen(combat, sound);
BuildingScreen buildingScreen(game, sound);
UpgradeScreen upgradeScreen(game, sound);
ZoneScreen zoneScreen(game, inventory, combat, sound, screenManager);

void setup()
{
    Serial.begin(115200);

    // Drop rolls and enemy picks come from random(); without a seed every boot
    // would roll the same sequence
    randomSeed(esp_random());

    menuButton.setup();
    confirmButton.setup();
    selectButton.setup();
    climate.setup();
    luminosity.setup();
    delay(100);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(0x0000);

    // This order is the navigation order, and each screen's Header labels name its
    // neighbours, so the two must be kept in step
    screenManager.addScreen(&miningScreen);
    screenManager.addScreen(&buildingScreen);
    screenManager.addScreen(&upgradeScreen);
    screenManager.addScreen(&inventoryScreen);
    screenManager.addScreen(&zoneScreen);
    screenManager.addScreen(&combatScreen);
}

void loop()
{
    unsigned long now = millis();

    menuButton.loop();
    confirmButton.loop();
    selectButton.loop();
    climate.loop(now);
    luminosity.loop(now);

    if (menuButton.wasPressed())
    {
        sound.playMenu();
        screenManager.nextScreen();
    }

    if (confirmButton.wasPressed())
    {
        screenManager.handleConfirmPress();
    }

    if (selectButton.wasPressed())
    {
        screenManager.handleSelectPress();
    }

    // Game progress runs every loop, whichever screen is visible
    game.update(now);
    combat.update(now);
    screenManager.update(now);
}
