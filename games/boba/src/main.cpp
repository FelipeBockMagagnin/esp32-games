#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Backlight.h"
#include "ButtonManager.h"
#include "Launcher.h"
#include "Settings.h"
#include "SlotStore.h"
#include "core/ScreenManager.h"
#include "core/SoundManager.h"
#include "pet/Pet.h"
#include "pet/Save.h"
#include "screens/ClockScreen.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameScreen.h"
#include "screens/HomeScreen.h"
#include "screens/SettingsScreen.h"
#include "ui/Panel.h"
#include "ui/Themes.h"

// The same carrier board as the idle game: a D-pad with Confirm in the middle
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

const unsigned long TICK_MS = 1000;
const unsigned long SETTINGS_HOLD_MS = 1500;
const unsigned long REPEAT_DELAY_MS = 400;
const unsigned long REPEAT_STEP_MS = 90;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button upButton = Button(UP_BUTTON_PIN);
Button downButton = Button(DOWN_BUTTON_PIN);
Button leftButton = Button(LEFT_BUTTON_PIN);

SlotStore store("boba");
Pet pet(esp_random());
AutoSaver saver(store, pet);
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);
LcdMatrix lcd;
Panel panel(lcd);

void resetAll();
void applySettings();
void petChanged();
ConfirmScreen resetScreen({"Reset", "Reset everything?", "The pet, the best age and the", "settings will be erased.",
                           "Resetting..."},
                          Theme::BAD, resetAll, screenManager, sound);
ClockScreen clockScreen(pet, lcd, panel, petChanged, screenManager, sound);
GameScreen gameScreen(pet, lcd, panel, petChanged, screenManager, sound);
HomeScreen homeScreen(pet, lcd, panel, petChanged, screenManager, sound, gameScreen);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, screenManager, sound, resetScreen, clockScreen);

unsigned long nextTickAt = 0;
bool wakeForCall = false;

void petChanged()
{
    saver.soon(millis());
}

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
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
    sound.setMode((SoundMode)settings.sound);
    saveSettings(store.getNamespace(), settings);
}

void resetAll()
{
    store.erase();
    ESP.restart();
}

// Holding Up and Down while the board powers on wipes the save
void loadOrReset()
{
    SaveBlob blob;
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readSave(store, blob))
    {
        pet.load(blob.pet);
        Serial.println("Save loaded");
    }
}

void handleEvents(unsigned long now)
{
    uint16_t events = pet.takeEvents();
    if (events == 0)
    {
        return;
    }
    if (events & (EVENT_HATCH | EVENT_EVOLVE | EVENT_SICK | EVENT_DEATH | EVENT_SLEEP | EVENT_WAKE))
    {
        saver.soon(now);
    }
    if (events & EVENT_DEATH)
    {
        sound.playDie();
    }
    else if (events & EVENT_EVOLVE)
    {
        sound.playEvolve();
    }
    else if (events & EVENT_HATCH)
    {
        sound.playHatch();
    }
    else if (events & EVENT_CALL)
    {
        sound.playCall();
    }
    if (events & (EVENT_CALL | EVENT_HATCH | EVENT_EVOLVE | EVENT_DEATH))
    {
        wakeForCall = true;
    }
    if (screenManager.getCurrentScreen() == &homeScreen)
    {
        homeScreen.handleEvents(events);
    }
}

// Time stands still only on the clock screen, or a change of hour would race the second it is on
void updateTime(unsigned long now)
{
    if (screenManager.getCurrentScreen() == &clockScreen)
    {
        nextTickAt = now + TICK_MS;
        return;
    }
    while ((long)(now - nextTickAt) >= 0)
    {
        nextTickAt += TICK_MS;
        pet.tick();
    }
}

void checkSettingsHold(unsigned long now)
{
    static unsigned long heldSince = 0;
    static bool holding = false;
    static bool fired = false;
    bool down = leftButton.isDown() && screenManager.getCurrentScreen() == &homeScreen && homeScreen.isIdle();
    if (!down)
    {
        holding = fired = false;
        return;
    }
    if (!holding)
    {
        holding = true;
        heldSince = now;
    }
    if (!fired && now - heldSince >= SETTINGS_HOLD_MS)
    {
        fired = true;
        sound.playMenu();
        screenManager.openModal(&settingsScreen);
    }
}

void checkRepeat(unsigned long now)
{
    static unsigned long nextRepeatAt = 0;
    static bool repeating = false;
    bool up = upButton.isDown();
    bool down = downButton.isDown();
    if (up == down)
    {
        repeating = false;
        return;
    }
    if (!repeating)
    {
        repeating = true;
        nextRepeatAt = now + REPEAT_DELAY_MS;
    }
    else if ((long)(now - nextRepeatAt) >= 0)
    {
        nextRepeatAt = now + REPEAT_STEP_MS;
        screenManager.handleRepeat(up ? Key::UP : Key::DOWN);
    }
}

#ifdef DEV_MODE
// Serial cheats for testing, and `shot` sends the panel to tools/screenshot.py
void sendScreen()
{
    static uint16_t row[TFT_WIDTH];
    int16_t w = tft.width(), h = tft.height();
    Serial.printf("SHOT %d %d\n", w, h);
    for (int16_t y = 0; y < h; y++)
    {
        tft.readRect(0, y, w, 1, row);
        Serial.write((const uint8_t *)row, w * sizeof(uint16_t));
    }
    Serial.println();
    Serial.println("END");
}

void devCommand(const char *line)
{
    Pet::Snapshot &s = pet.raw();
    if (strncmp(line, "screen ", 7) == 0)
    {
        screenManager.setScreen(atoi(line + 7));
    }
    else if (strcmp(line, "shot") == 0)
    {
        sendScreen();
    }
    else if (strncmp(line, "form ", 5) == 0)
    {
        s.form = (uint8_t)(atoi(line + 5) % FORM_COUNT);
        s.stageSeconds = 0;
        s.lifeDays = 30;
    }
    else if (strncmp(line, "hunger ", 7) == 0)
    {
        s.hunger = (uint8_t)atoi(line + 7);
    }
    else if (strncmp(line, "happy ", 6) == 0)
    {
        s.happy = (uint8_t)atoi(line + 6);
    }
    else if (strncmp(line, "poop ", 5) == 0)
    {
        s.poops = (uint8_t)atoi(line + 5);
    }
    else if (strcmp(line, "sick") == 0)
    {
        s.doses = 1;
    }
    else if (strncmp(line, "age ", 4) == 0)
    {
        s.ageSeconds = (uint32_t)atoi(line + 4) * DAY_SECONDS;
    }
    else if (strncmp(line, "clock ", 6) == 0)
    {
        pet.setClock((uint32_t)atoi(line + 6) * 3600);
    }
    else if (strncmp(line, "theme ", 6) == 0)
    {
        settings.theme = (uint8_t)atoi(line + 6);
        applySettings();
        screenManager.repaint();
    }
    else if (strncmp(line, "skip ", 5) == 0)
    {
        for (int i = atoi(line + 5); i > 0; i--)
        {
            pet.tick();
        }
    }
    else if (line[0] != '\0')
    {
        Serial.println("screen <i> | shot | form <i> | hunger <n> | happy <n> | poop <n> | sick | age <y> | clock <h> | theme <i> | skip <s>");
    }
}

void devConsole()
{
    static char line[32];
    static uint8_t length = 0;
    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c != '\n' && c != '\r')
        {
            if (length < sizeof(line) - 1)
            {
                line[length++] = c;
            }
            continue;
        }
        line[length] = '\0';
        length = 0;
        devCommand(line);
    }
}
#endif

void setup()
{
    Serial.begin(115200);
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

    loadOrReset();
    loadSettings(store.getNamespace(), settings);
    applySettings();

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::FIELD);
    backlight.begin();

    // screenshot.py's --screen counts on this order
    screenManager.addScreen(&homeScreen, "Boba");
    screenManager.openModal(&clockScreen);
    backlight.setFull();
    saver.begin(millis());
    nextTickAt = millis() + TICK_MS;
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
    bool woke = wakeForCall;
    wakeForCall = false;
    if (backlight.update(now, right || confirm || up || down || left || woke, held, DIM_AFTER_MS[settings.dimAfter]))
    {
        right = confirm = up = down = left = false;
    }
#ifdef DEV_MODE
    devConsole();
#endif

    updateTime(now);
    handleEvents(now);
    checkSettingsHold(now);
    checkRepeat(now);

    if (up)
    {
        screenManager.handleUpPress();
    }
    if (down)
    {
        screenManager.handleDownPress();
    }
    if (left)
    {
        screenManager.handleBackPress();
    }
    if (right)
    {
        screenManager.handleRightPress();
    }
    if (confirm)
    {
        screenManager.handleConfirmPress();
    }

    screenManager.update(now);
    saver.update(now);
}
