#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "pitches.h"
#include "button.h"
#include "./managers/ScreenManager.h"
#include "screens/HomeScreen.h"
#include "screens/InventoryScreen.h"

// Screen Dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

const int BUTTON_PIN = 4;
const int BUZZER_PIN = 32;

TFT_eSPI tft = TFT_eSPI();
Button btn = Button(BUTTON_PIN);

ScreenManager screenManager(tft);
HomeScreen homeScreen;
InventoryScreen inventoryScreen;

void setup()
{
    Serial.begin(115200);

    btn.setup();
    delay(100);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(0x0000);

    screenManager.addScreen(&homeScreen);
    screenManager.addScreen(&inventoryScreen);
}

void loop()
{
    unsigned long now = millis();

    btn.loop();

    if (btn.wasPressed())
    {
        tone(BUZZER_PIN, NOTE_GS7, 100);
        screenManager.nextScreen();
    }

    screenManager.update(now);
}
