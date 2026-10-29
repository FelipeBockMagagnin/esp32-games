#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "ButtonManager.h"
#include "Launcher.h"
#include "managers/ScreenManager.h"
#include "managers/SoundManager.h"
#include "game/World.h"
#include "game/Colony.h"
#include "game/SaveGame.h"
#include "screens/HabitatScreen.h"
#include "screens/BuildMenuScreen.h"
#include "screens/ColonyScreen.h"
#include "screens/ShopScreen.h"
#include "screens/ConfirmScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SplashScreen.h"
#include "ui/Themes.h"
#include "debug/DevConsole.h"

// The buttons form a D-pad: Confirm in the middle, the arrows around it. Right/Left step
// through the screens, except on the habitat, where they move the cursor.
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button upButton = Button(UP_BUTTON_PIN);
Button downButton = Button(DOWN_BUTTON_PIN);
Button leftButton = Button(LEFT_BUTTON_PIN);

// Globals in one translation unit initialize in declaration order: the colony stands on the world
World world;
Colony colony(world);
SoundManager sound(BUZZER_PIN);
ScreenManager screenManager(tft);

BuildMenuScreen buildMenu(world, screenManager, sound);
HabitatScreen habitatScreen(world, colony, sound, buildMenu);
ColonyScreen colonyScreen(world, colony, sound);
ShopScreen shopScreen(world, colony, sound);

void resetGame();
void applySettings();
void openGameMenu();
ConfirmScreen resetScreen({"Reset", "Reset game?", "The habitat and every critter", "will be lost.",
                           "Erase the save and restart", "Resetting..."},
                          Theme::BAD, resetGame, screenManager, sound);
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SettingsScreen settingsScreen(settings, applySettings, openGameMenu, screenManager, sound, resetScreen);
SplashScreen splashScreen(screenManager, sound);

AutoSaver autoSaver(world, colony);
SaveBlob saveBuffer;

#ifdef DEV_MODE
void devChanged()
{
    screenManager.repaint();
}
DevConsole devConsole(world, colony, devChanged);
#endif

// Last Colony event count the loop showed a notice for
uint32_t seenColonyEvents = 0;
static const unsigned long NOTICE_MS = 4000;
unsigned long noticeUntil = 0;
const char *noticeCaption = nullptr;
char noticeCaptionText[24];
Color noticeAccent = Theme::GOOD;

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

void openGameMenu()
{
    captureSave(saveBuffer, world, colony);
    if (!saveGame(saveBuffer))
    {
        Serial.println("Save failed");
    }
    returnToLauncher();
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

// Not on the habitat, where a held Left sweeps the cursor instead
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

// A held arrow repeats on screens that move a cursor with them, starting slow so a tap
// stays a single step. One arrow at a time: the last one pressed.
static const unsigned long ARROW_DELAY_MS = 300;
static const unsigned long ARROW_STEP_MS = 90;
Button *const ARROW_BUTTONS[] = {&upButton, &downButton, &leftButton, &rightButton};
int8_t repeatingArrow = -1;
unsigned long arrowRepeatAt = 0;

void checkArrowRepeat(unsigned long now, const bool pressed[4])
{
    for (uint8_t a = 0; a < 4; a++)
    {
        if (pressed[a])
        {
            repeatingArrow = a;
            arrowRepeatAt = now + ARROW_DELAY_MS;
        }
    }
    if (repeatingArrow < 0)
    {
        return;
    }
    if (!ARROW_BUTTONS[repeatingArrow]->isDown() || !screenManager.repeatsArrows())
    {
        repeatingArrow = -1;
        return;
    }
    if ((long)(now - arrowRepeatAt) < 0)
    {
        return;
    }
    arrowRepeatAt = now + ARROW_STEP_MS;
    if (!screenManager.handleArrowRepeat((Arrow)repeatingArrow))
    {
        repeatingArrow = -1;
    }
}

// Births and departures, announced on whichever screen is open
void showColonyNotices(unsigned long now)
{
    if (colony.getEventCount() != seenColonyEvents)
    {
        seenColonyEvents = colony.getEventCount();
        const char *species = SPECIES[colony.getLastEventSpecies()].name;
        if (colony.getLastEvent() == ColonyEvent::BORN)
        {
            snprintf(noticeCaptionText, sizeof(noticeCaptionText), "A %s IS BORN", species);
            noticeAccent = Theme::GOOD;
            sound.playBirth();
        }
        else
        {
            snprintf(noticeCaptionText, sizeof(noticeCaptionText), "LEFT, HUNGRY");
            noticeAccent = Theme::BAD;
            sound.playLeave();
        }
        for (char *p = noticeCaptionText; *p != '\0'; p++)
        {
            *p = toupper(*p);
        }
        noticeCaption = noticeCaptionText;
        noticeUntil = now + NOTICE_MS;
    }

    Screen *current = screenManager.getCurrentScreen();
    if (current == nullptr)
    {
        return;
    }
    if ((long)(now - noticeUntil) < 0)
    {
        current->applyNotice(noticeCaption, colony.getLastEventName(), noticeAccent);
    }
    else
    {
        current->applyNotice(nullptr, nullptr, Theme::GOOD);
    }
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
        applySave(saveBuffer, world, colony);
        Serial.println("Save loaded");
        return true;
    }
    Serial.println("No save, starting fresh");
    return false;
}

void setup()
{
    Serial.begin(115200);

    // Wandering and breeding roll random(); without a seed every boot would roll the same
    randomSeed(esp_random());

    rightButton.setup();
    confirmButton.setup();
    upButton.setup();
    downButton.setup();
    leftButton.setup();
    delay(100);

    // Holding Confirm while the board powers on opens the game menu, if the site installed one:
    // the way out should a game fail before its Settings
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }

    bool loaded = loadOrReset();
    loadSettings(settings);
    applySettings();
    seenColonyEvents = colony.getEventCount();
    autoSaver.begin(millis(), loaded ? &saveBuffer : nullptr);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::BG);
    ledcSetup(BACKLIGHT_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, BACKLIGHT_CHANNEL);

    screenManager.addScreen(&habitatScreen, "Habitat");
    screenManager.addScreen(&colonyScreen, "Colony");
    screenManager.addScreen(&shopScreen, "Shop");

    // Lit only once the splash covers the habitat that addScreen() just painted
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
#ifdef DEV_MODE
    devConsole.update();
#endif

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
    const bool arrows[4] = {up, down, left, right};
    checkArrowRepeat(now, arrows);

    // The habitat lives whichever screen is visible
    world.update(now);
    colony.update(now);

    showColonyNotices(now);
    screenManager.update(now);
    autoSaver.update(now);
}
