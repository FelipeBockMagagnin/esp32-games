#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "core/Backlight.h"
#include "core/ButtonManager.h"
#include "core/ScreenManager.h"
#include "core/Settings.h"
#include "core/SlotStore.h"
#include "core/SoundManager.h"
#include "rogue/Run.h"
#include "rogue/Save.h"
#include "screens/ConfirmScreen.h"
#include "screens/DungeonScreen.h"
#include "screens/GameOverScreen.h"
#include "screens/MenuScreen.h"
#include "screens/SettingsScreen.h"
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

SlotStore store("roguelike");
Run run;
Records records = {};
SaveBlob saveBuffer;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

void resetGame();
void applySettings();
void floorReached();
void died();
void newRun();
ConfirmScreen resetScreen({"Reset", "Reset game?", "The run and the records", "will be erased.",
                           "Erase the save and restart", "Resetting..."},
                          Theme::BAD, resetGame, screenManager, sound);
SettingsScreen settingsScreen(settings, applySettings, screenManager, sound, resetScreen);
MenuScreen menuScreen(run, screenManager, sound, settingsScreen);
DungeonScreen dungeonScreen(run, sound, screenManager, menuScreen, floorReached, died);
GameOverScreen gameOverScreen(run, records, sound, newRun);

void saveNow()
{
    captureSave(saveBuffer, run, records);
    if (!writeSave(store, saveBuffer))
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

void resetGame()
{
    store.erase();
    ESP.restart();
}

// Saved only at the start of a floor: a power cut mid-floor replays it from its start
void floorReached()
{
    saveNow();
}

void died()
{
    gameOverScreen.setNewBest(recordRun(records, run));
    saveNow();
    screenManager.openModal(&gameOverScreen);
}

void newRun()
{
    run.start(esp_random());
    saveNow();
    sound.playStart();
    screenManager.closeModal();
}

// Holding a direction keeps walking, at a pace the eye can follow
static const unsigned long REPEAT_DELAY_MS = 300;
static const unsigned long REPEAT_STEP_MS = 130;
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

// Holding Up and Down while the board powers on wipes the save
void loadOrReset()
{
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readSave(store, saveBuffer))
    {
        records = saveBuffer.records;
        run.load(saveBuffer.run);
        Serial.println("Save loaded");
    }
    if (!run.isActive())
    {
        run.start(esp_random());
        saveNow();
    }
}

#ifdef DEV_MODE
// Serial commands for testing: `floor <n>` jumps there with what the player carries, `shot`
// sends the panel to tools/screenshot.py
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
        if (strncmp(line, "floor ", 6) == 0 && atoi(line + 6) > 0)
        {
            Run::Snapshot snap;
            run.save(snap);
            snap.floor = (uint8_t)atoi(line + 6);
            run.load(snap);
            screenManager.repaint();
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (line[0] != '\0')
        {
            Serial.println("floor <n> | shot");
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

    loadOrReset();
    loadSettings(store.getNamespace(), settings);
    applySettings();

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::BG);
    backlight.begin();

    menuScreen.setDungeon(&dungeonScreen);
    screenManager.addScreen(&dungeonScreen, "Dungeon");
    backlight.setFull();
    sound.playStart();
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
        screenManager.handleBackPress();
        startRepeat(now, Key::LEFT);
    }
    else if (right)
    {
        screenManager.handleRightPress();
        startRepeat(now, Key::RIGHT);
    }
    else if (confirm)
    {
        screenManager.handleConfirmPress();
        startRepeat(now, Key::CONFIRM);
    }
    checkRepeat(now);

    screenManager.update(now);
}
