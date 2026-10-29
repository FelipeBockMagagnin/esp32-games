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
#include "screens/ConfirmScreen.h"
#include "screens/PlayScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/TitleScreen.h"
#include "mines/Field.h"
#include "mines/Records.h"
#include "ui/Themes.h"

// The same carrier board as the idle game: a D-pad with Confirm in the middle
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

SlotStore store("mines");
Records records;
Field field;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

uint32_t hardwareRandom()
{
    return esp_random();
}

bool confirmDown()
{
    return confirmButton.isDown();
}

void resetAll();
void applySettings();
void saveRecords();
ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every best time, win and", "setting will be erased.",
                           "Resetting..."},
                          Theme::BAD, resetAll, screenManager, sound);
PlayScreen playScreen(tft, field, records, hardwareRandom, saveRecords, confirmDown, screenManager, sound);
TitleScreen titleScreen(tft, records, hardwareRandom, saveRecords, settings, applySettings, screenManager, sound,
                        playScreen);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, screenManager, sound, resetScreen);

void saveRecords()
{
    if (!writeRecords(store, records))
    {
        Serial.println("Save failed");
    }
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

// Holding an arrow walks the cursor across the field
static const unsigned long REPEAT_DELAY_MS = 300;
static const unsigned long REPEAT_STEP_MS = 70;
Button *const KEY_BUTTONS[] = {&upButton, &downButton, &leftButton, &rightButton, &confirmButton};
bool repeating = false;
Key repeatKey = Key::UP;
unsigned long repeatAt = 0;

void startRepeat(unsigned long now, Key key)
{
    repeating = true;
    repeatKey = key;
    repeatAt = now + REPEAT_DELAY_MS;
}

void checkRepeat(unsigned long now)
{
    if (!repeating)
    {
        return;
    }
    if (!KEY_BUTTONS[(uint8_t)repeatKey]->isDown())
    {
        repeating = false;
        return;
    }
    if ((long)(now - repeatAt) < 0)
    {
        return;
    }
    if (!screenManager.handleRepeat(repeatKey))
    {
        repeating = false;
        return;
    }
    repeatAt = now + REPEAT_STEP_MS;
}

// Holding Up and Down while the board powers on wipes the records and settings
void loadOrReset()
{
    defaultRecords(records);
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readRecords(store, records))
    {
        Serial.println("Save loaded");
    }
}

#ifdef DEV_MODE
// Serial commands for testing: `screen <i>` switches screen, `shot` sends the panel to
// tools/screenshot.py
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
        if (strncmp(line, "screen ", 7) == 0)
        {
            screenManager.setScreen(atoi(line + 7));
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (line[0] != '\0')
        {
            Serial.println("screen <i> | shot");
        }
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
    tft.fillScreen(Theme::BG);
    backlight.begin();

    // screenshot.py's --screen counts on this order
    screenManager.addScreen(&titleScreen, "Mines");
    screenManager.addScreen(&settingsScreen, "Settings");
    backlight.setFull();
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
    if (backlight.update(now, right || confirm || up || down || left, held, DIM_AFTER_MS[settings.dimAfter]))
    {
        right = confirm = up = down = left = false;
    }
#ifdef DEV_MODE
    devConsole();
#endif

    if (up)
    {
        screenManager.handleUpPress();
        startRepeat(now, Key::UP);
    }
    else if (down)
    {
        screenManager.handleDownPress();
        startRepeat(now, Key::DOWN);
    }
    else if (left)
    {
        if (!screenManager.handleBackPress())
        {
            sound.playMenu();
        }
        startRepeat(now, Key::LEFT);
    }
    else if (right)
    {
        if (!screenManager.handleRightPress())
        {
            sound.playMenu();
        }
        startRepeat(now, Key::RIGHT);
    }
    else if (confirm)
    {
        screenManager.handleConfirmPress();
    }
    checkRepeat(now);

    screenManager.update(now);
}
