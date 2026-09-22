#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "pitches.h"
#include "managers/ButtonManager.h"
#include "managers/ScreenManager.h"
#include "game/GameState.h"
#include "managers/SoundManager.h"
#include "screens/InventoryScreen.h"
#include "screens/MiningScreen.h"
#include "screens/CombatScreen.h"
#include "screens/BuildingScreen.h"
#include "screens/UpgradeScreen.h"

// Screen Dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

const int MENU_BUTTON_PIN = 23;    // Switches to the next screen
const int CONFIRM_BUTTON_PIN = 27; // Confirms on the current screen: mines / buys
const int SELECT_BUTTON_PIN = 22;  // Selects the next item inside the current screen
const int BUZZER_PIN = 32;

TFT_eSPI tft = TFT_eSPI();
Button menuButton = Button(MENU_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button selectButton = Button(SELECT_BUTTON_PIN);

GameState game;
SoundManager sound(BUZZER_PIN);
ScreenManager screenManager(tft);

InventoryScreen inventoryScreen;
MiningScreen miningScreen(game, sound);
CombatScreen combatScreen;
BuildingScreen buildingScreen(game, sound);
UpgradeScreen upgradeScreen(game, sound);

void setup()
{
    Serial.begin(115200);

    menuButton.setup();
    confirmButton.setup();
    selectButton.setup();
    delay(100);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(0x0000);

    screenManager.addScreen(&miningScreen);
    screenManager.addScreen(&buildingScreen);
    screenManager.addScreen(&upgradeScreen);
    screenManager.addScreen(&inventoryScreen);
    screenManager.addScreen(&combatScreen);
}

void loop()
{
    unsigned long now = millis();

    menuButton.loop();
    confirmButton.loop();
    selectButton.loop();

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
    screenManager.update(now);
}
