#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Backlight.h"
#include "ButtonManager.h"
#include "Launcher.h"
#include "Settings.h"
#include "core/ScreenManager.h"
#include "core/SoundManager.h"
#include "play/Cart.h"
#include "play/Crisp.h"
#include "play/Host.h"
#include "save/Scores.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameListScreen.h"
#include "screens/SettingsScreen.h"
#include "ui/Themes.h"

const int RIGHT_BUTTON_PIN = 19;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 26;
const int DOWN_BUTTON_PIN = 23;
const int LEFT_BUTTON_PIN = 22;
const int BUZZER_PIN = 32;

// Read once a frame during a game: the library's 50 ms would lose a quick tap
static const unsigned long GAME_DEBOUNCE_MS = 15;
const char *const NAMESPACE = "crisp";

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button *const KEY_BUTTONS[] = {&upButton, &downButton, &leftButton, &rightButton, &confirmButton};

Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

void resetAll();
void applySettings();
void playGame(uint16_t index);
void quitGame(uint16_t index);
void storeScores(uint16_t index);
ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every best score and the", "settings will be erased.",
                           "Resetting..."},
                          Theme::BAD, resetAll, screenManager, sound);
GameListScreen listScreen(playGame, sound);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, screenManager, sound, resetScreen);
Host host(tft, {upButton, downButton, leftButton, rightButton, confirmButton}, sound, quitGame, storeScores);

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
    saveSettings(NAMESPACE, settings);
}

void eraseSaves()
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.clear();
        prefs.end();
    }
}

void resetAll()
{
    eraseSaves();
    ESP.restart();
}

void playGame(uint16_t index)
{
    if (index >= CART_COUNT)
    {
        return;
    }
    sound.playStart();
    backlight.setFull();
    host.start(index);
}

void quitGame(uint16_t index)
{
    sound.playNote(0);
    listScreen.setSelected(index);
    screenManager.repaint();
}

void storeScores(uint16_t)
{
    Scores::store(NAMESPACE);
}

// Holding a button repeats it while the list scrolls, faster the longer it is held: the list
// is long
static const unsigned long REPEAT_DELAY_MS = 350;
static const unsigned long REPEAT_STEP_MS = 80;
static const unsigned long REPEAT_FAST_MS = 25;
static const uint8_t REPEAT_FAST_AFTER = 12;
uint8_t repeats = 0;
bool repeating = false;
Key repeatKey = Key::UP;
unsigned long repeatAt = 0;

void startRepeat(unsigned long now, Key key)
{
    repeating = true;
    repeatKey = key;
    repeatAt = now + REPEAT_DELAY_MS;
    repeats = 0;
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
    if (repeats < REPEAT_FAST_AFTER)
    {
        repeats++;
    }
    repeatAt = now + (repeats < REPEAT_FAST_AFTER ? REPEAT_STEP_MS : REPEAT_FAST_MS);
}

#ifdef DEV_MODE
// Serial commands for testing: `screen <i>` switches screen, `play <i>` starts a game
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
        else if (strncmp(line, "play ", 5) == 0)
        {
            playGame((uint16_t)atoi(line + 5));
        }
        else if (line[0] != '\0')
        {
            Serial.println("screen <i> | play <i>");
        }
    }
}
#endif

void setup()
{
    Serial.begin(115200);

    for (Button *b : KEY_BUTTONS)
    {
        b->setup();
    }
    delay(100);

    // Holding Confirm while the board powers on opens the game menu, if the site installed one:
    // the way out should a game fail before its menu
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }
    // Holding Up and Down while it powers on erases every score and the settings
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        eraseSaves();
        Serial.println("Saves erased");
    }

    loadSettings(NAMESPACE, settings);
    applySettings();
    crispInit();
    Scores::load(NAMESPACE);
    Serial.printf("%u games, %u bytes of heap free, %u in one block\n", (unsigned)CART_COUNT,
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());

    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(Theme::BG);
    if (!host.begin())
    {
        Serial.println("No DMA: drawing without it");
    }
    backlight.begin();
    backlight.setFull();

    screenManager.addScreen(&listScreen, "Games");
    screenManager.addScreen(&settingsScreen, "Settings");
}

void loop()
{
    if (host.running())
    {
        host.loop();
        return;
    }

    unsigned long now = millis();
    for (Button *b : KEY_BUTTONS)
    {
        b->loop();
    }
    bool right = rightButton.wasPressed();
    bool confirm = confirmButton.wasPressed();
    bool up = upButton.wasPressed();
    bool down = downButton.wasPressed();
    bool left = leftButton.wasPressed();
    bool held = rightButton.isDown() || confirmButton.isDown() || upButton.isDown() || downButton.isDown() ||
                leftButton.isDown();
    bool woke = backlight.update(now, right || confirm || up || down || left, held, DIM_AFTER_MS[settings.dimAfter]);
    if (woke)
    {
        right = confirm = up = down = left = false;
    }
#ifdef DEV_MODE
    devConsole();
#endif

    checkRepeat(now);
    if (up)
    {
        screenManager.handleUpPress();
        startRepeat(now, Key::UP);
    }
    if (down)
    {
        screenManager.handleDownPress();
        startRepeat(now, Key::DOWN);
    }
    if (left && !screenManager.handleBackPress())
    {
        sound.playMenu();
    }
    if (right && !screenManager.handleRightPress())
    {
        sound.playMenu();
    }
    if (confirm)
    {
        screenManager.handleConfirmPress();
        if (host.running())
        {
            return;
        }
    }

    screenManager.update(now);
}
