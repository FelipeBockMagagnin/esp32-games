#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "ButtonManager.h"
#include "managers/ScreenManager.h"
#include "managers/SoundManager.h"
#include "game/SaveGame.h"
#include "screens/TitleScreen.h"
#include "screens/LevelScreen.h"
#include "screens/ConfirmScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SplashScreen.h"
#include "ui/Themes.h"

// The board is held on its side, turned a quarter clockwise from the other games, so the D-pad
// sits under the left thumb. Each button is named for the way it points held like that: the
// arrow the other games call Up now points right.
static const uint8_t ROTATION = 1;
const int RIGHT_BUTTON_PIN = 19;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 26;
const int DOWN_BUTTON_PIN = 23;
const int LEFT_BUTTON_PIN = 22;
const int BUZZER_PIN = 32;

// The engine's 50 ms is felt in a jump; the switches settle well inside this
static const unsigned long DEBOUNCE_MS = 15;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN, DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, DEBOUNCE_MS);

SoundManager sound(BUZZER_PIN);
ScreenManager screenManager(tft);
SaveBlob records;

void resetGame();
void applySettings();
void startFloor(uint8_t level);
void backToTitle();
ConfirmScreen resetScreen({"Reset", "Reset game?", "Every best time will be lost.", "",
                           "Erase the save and restart", "Resetting..."},
                          Theme::BAD, resetGame, screenManager, sound);
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SettingsScreen settingsScreen(settings, applySettings, screenManager, sound, resetScreen);
SplashScreen splashScreen(screenManager, sound);
TitleScreen titleScreen(screenManager, sound, records, settingsScreen, startFloor);
LevelScreen levelScreen(screenManager, sound, records, settingsScreen, backToTitle);

static const int TITLE_INDEX = 0;
static const int LEVEL_INDEX = 1;

// tone() drives LEDC channel 0, and channel 1 shares its timer
static const uint8_t BACKLIGHT_CHANNEL = 4;
static const uint8_t BACKLIGHT_FULL = 255;
static const uint8_t BACKLIGHT_DIM = 20;
unsigned long lastInputAt = 0;
bool dimmed = false;

void applySettings()
{
    if (settings.sound >= (uint8_t)SoundMode::COUNT)
    {
        settings.sound = (uint8_t)SoundMode::ON;
    }
    if (settings.dimAfter >= DIM_OPTION_COUNT)
    {
        settings.dimAfter = DIM_DEFAULT;
    }
    if (settings.theme >= THEME_COUNT)
    {
        settings.theme = 0;
    }
    Theme::setActive(settings.theme);
    sound.setMode((SoundMode)settings.sound);
    saveSettings(settings);
}

void startFloor(uint8_t level)
{
    levelScreen.begin(level);
    screenManager.setScreen(LEVEL_INDEX);
}

void backToTitle()
{
    screenManager.setScreen(TITLE_INDEX);
}

// True when the press only woke the screen, so it must not reach the game
bool wakeOnInput(unsigned long now, bool pressed, bool held)
{
    if (pressed || held)
    {
        lastInputAt = now;
    }
    if (dimmed && pressed)
    {
        dimmed = false;
        ledcWrite(BACKLIGHT_CHANNEL, BACKLIGHT_FULL);
        return true;
    }
    unsigned long dimAfter = DIM_AFTER_MS[settings.dimAfter];
    if (!dimmed && dimAfter > 0 && now - lastInputAt >= dimAfter)
    {
        dimmed = true;
        ledcWrite(BACKLIGHT_CHANNEL, BACKLIGHT_DIM);
    }
    return false;
}

void resetGame()
{
    eraseSave();
    ESP.restart();
}

bool inLevel()
{
    return screenManager.getCurrentScreen() == &levelScreen;
}

// Holding Up and Down this long asks whether to reset. Not during a floor, where every arrow plays.
static const unsigned long RESET_HOLD_MS = 3000;
unsigned long resetHoldStart = 0;
bool resetHolding = false;
bool resetHoldFired = false; // Latched until a button is released, so one hold opens it once

void checkResetGesture(unsigned long now)
{
    if (!upButton.isDown() || !downButton.isDown() || inLevel())
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

// Not during a floor either, where a held Left runs; Settings is in its pause menu
static const unsigned long SETTINGS_HOLD_MS = 1500;
unsigned long settingsHoldStart = 0;
bool settingsHolding = false;
bool settingsHoldFired = false;

void checkSettingsGesture(unsigned long now)
{
    if (!leftButton.isDown() || upButton.isDown() || downButton.isDown() || screenManager.repeatsArrows())
    {
        settingsHolding = false;
        settingsHoldFired = false;
        return;
    }
    if (!settingsHolding)
    {
        settingsHolding = true;
        settingsHoldStart = now;
    }
    if (!settingsHoldFired && now - settingsHoldStart >= SETTINGS_HOLD_MS && !screenManager.hasModal())
    {
        settingsHoldFired = true;
        sound.playMenu();
        screenManager.openModal(&settingsScreen);
    }
}

// Holding Up and Down while the board powers on wipes the save
void loadOrReset()
{
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        eraseSave();
        Serial.println("Save erased");
    }
    else if (loadGame(records))
    {
        Serial.println("Save loaded");
        return;
    }
    newSave(records);
}

void setup()
{
    Serial.begin(115200);

    rightButton.setup();
    confirmButton.setup();
    upButton.setup();
    downButton.setup();
    leftButton.setup();
    delay(100);

    loadOrReset();
    loadSettings(settings);
    applySettings();

    tft.init();
    tft.setRotation(ROTATION);
    tft.initDMA();
    tft.fillScreen(Theme::BG);
    ledcSetup(BACKLIGHT_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, BACKLIGHT_CHANNEL);

    screenManager.addScreen(&titleScreen, "Title");
    screenManager.addScreen(&levelScreen, "Floor");

    // Lit only once the splash covers the title that addScreen() just painted
    screenManager.openModal(&splashScreen);
    ledcWrite(BACKLIGHT_CHANNEL, BACKLIGHT_FULL);
}

void loop()
{
    unsigned long now = millis();

    rightButton.loop();
    confirmButton.loop();
    upButton.loop();
    downButton.loop();
    leftButton.loop();

    bool right = rightButton.wasPressed();
    bool confirm = confirmButton.wasPressed();
    bool up = upButton.wasPressed();
    bool down = downButton.wasPressed();
    bool left = leftButton.wasPressed();
    bool held = rightButton.isDown() || confirmButton.isDown() || upButton.isDown() || downButton.isDown() ||
                leftButton.isDown();
    if (wakeOnInput(now, right || confirm || up || down || left, held))
    {
        right = confirm = up = down = left = false;
    }

    checkResetGesture(now);
    checkSettingsGesture(now);

    if (right && screenManager.handleNextPress())
    {
        sound.playMenu();
    }
    if (confirm)
    {
        screenManager.handleConfirmPress();
    }
    if (up)
    {
        screenManager.handleUpPress();
    }
    if (down)
    {
        screenManager.handleDownPress();
    }
    if (left && !screenManager.handleBackPress())
    {
        sound.playMenu();
    }

    levelScreen.setHeld(leftButton.isDown(), rightButton.isDown(), confirmButton.isDown());
    screenManager.update(now);
}
