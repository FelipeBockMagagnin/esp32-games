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
#include "crawl/Run.h"
#include "crawl/Save.h"
#include "render/Frame.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameOverScreen.h"
#include "screens/InventoryScreen.h"
#include "screens/PauseScreen.h"
#include "screens/PlayScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/TitleScreen.h"
#include "ui/Themes.h"

// Held on its side like Decaf, a quarter turn clockwise from the other games, so the D-pad sits
// under the left thumb. Each button is named for the way it points held like that.
static const uint8_t ROTATION = 1;
const int RIGHT_BUTTON_PIN = 19;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 26;
const int DOWN_BUTTON_PIN = 23;
const int LEFT_BUTTON_PIN = 22;
const int BUZZER_PIN = 32;

// A quick tap of Confirm is a sword blow; the engine's 50 ms would swallow some
static const unsigned long DEBOUNCE_MS = 15;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN, DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, DEBOUNCE_MS);

SlotStore store("grimhold");
SaveBlob blob;
Records records;
Run run;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);
Frame frame(tft);

void resetAll();
void applySettings();
void continueRun();
void askNewRun();
void openSettings();
void restartRun();
void abandonRun();
void descended();
void died();
void paused();
void taught();

ConfirmScreen resetScreen({"Reset", "Reset everything?", "The run, the records and the", "settings will be erased.",
                           "Resetting..."},
                          Theme::BAD, resetAll, screenManager, sound);
ConfirmScreen restartScreen({"New run", "Start over?", "The saved run will be lost,", "counted as a fall.",
                             "Starting..."},
                            Theme::BAD, restartRun, screenManager, sound);
ConfirmScreen abandonScreen({"Abandon", "Abandon the run?", "It ends here, as if you fell.", "",
                             "Abandoning..."},
                            Theme::BAD, abandonRun, screenManager, sound);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, screenManager, sound, resetScreen);
PlayScreen playScreen(run, frame, sound, {descended, died, paused, taught});
GameOverScreen gameOverScreen(screenManager, sound);
// The pause menu and the inventory lead to each other
extern InventoryScreen inventoryScreen;
PauseScreen pauseScreen(run, screenManager, sound, playScreen, inventoryScreen, settingsScreen, abandonScreen);
InventoryScreen inventoryScreen(run, screenManager, sound, pauseScreen);
TitleScreen titleScreen(sound, blob, {continueRun, askNewRun, openSettings});

void saveNow()
{
    captureSave(blob, run, records);
    if (!writeSave(store, blob))
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

void play()
{
    playScreen.begin(!records.tutorialDone);
    screenManager.replaceModal(&playScreen);
}

void startNewRun()
{
    run.newRun(esp_random());
    saveNow();
    play();
}

void continueRun()
{
    run.restore(blob.run);
    play();
}

void askNewRun()
{
    if (blob.run.floor != 0)
    {
        screenManager.openModal(&restartScreen);
    }
    else
    {
        startNewRun();
    }
}

void openSettings()
{
    settingsScreen.setReturnTo(nullptr);
    screenManager.openModal(&settingsScreen);
}

void restartRun()
{
    run.restore(blob.run);
    recordRun(records, run);
    startNewRun();
}

void abandonRun()
{
    recordRun(records, run);
    run.end();
    saveNow();
    screenManager.closeModal();
}

void descended()
{
    saveNow();
}

void died()
{
    bool deepest = recordRun(records, run);
    saveNow();
    gameOverScreen.setResult(run.getFloor(), run.getKills(), deepest);
    screenManager.replaceModal(&gameOverScreen);
}

void paused()
{
    settingsScreen.setReturnTo(&pauseScreen);
    screenManager.replaceModal(&pauseScreen);
}

void taught()
{
    records.tutorialDone = 1;
    saveNow();
}

void pollButtons()
{
    rightButton.loop();
    confirmButton.loop();
    upButton.loop();
    downButton.loop();
    leftButton.loop();
}

// Holding Up and Down while the board powers on wipes the run, the records and the settings
void loadOrReset()
{
    memset(&records, 0, sizeof(records));
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readSave(store, blob))
    {
        records = blob.records;
        Serial.println("Save loaded");
        return;
    }
    memset(&blob, 0, sizeof(blob));
}

#ifdef DEV_MODE
// Serial commands for testing: `fps` prints the frame times once a second until sent again,
// `next` goes down a floor, `shot` sends the panel to tools/screenshot.py
bool printFps = false;

void sendScreen()
{
    static uint16_t row[TFT_HEIGHT];
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

void devConsole(unsigned long now)
{
    static char line[32];
    static uint8_t length = 0;
    static unsigned long lastFps = 0;
    if (printFps && now - lastFps >= 1000)
    {
        lastFps = now;
        Serial.printf("frame %lu us, drawing %lu us, heap %u\n", (unsigned long)frame.getFrameMicros(),
                      (unsigned long)frame.getDrawMicros(), (unsigned)ESP.getFreeHeap());
    }
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
        if (strcmp(line, "fps") == 0)
        {
            printFps = !printFps;
        }
        else if (strcmp(line, "next") == 0)
        {
            run.nextFloor();
            saveNow();
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (line[0] != '\0')
        {
            Serial.println("fps | next | shot");
        }
    }
}
#endif

void setup()
{
    Serial.begin(115200);

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
    resetScreen.setReturnTo(&settingsScreen);
    abandonScreen.setReturnTo(&pauseScreen);

    tft.init();
    tft.setRotation(ROTATION);
    tft.initDMA();
    tft.fillScreen(Theme::BG);
    backlight.begin();
    frame.setPoll(pollButtons);

    screenManager.addScreen(&titleScreen, "Grimhold");
    backlight.setFull();
    sound.playTitle();
}

void loop()
{
    unsigned long now = millis();

    pollButtons();
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
    devConsole(now);
#endif

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

    // A tap shorter than a frame still reaches the run as one frame held
    playScreen.setHeld(upButton.isDown(), downButton.isDown(), leftButton.isDown(), rightButton.isDown(),
                       confirmButton.isDown() || confirm);
    screenManager.update(now);
}
