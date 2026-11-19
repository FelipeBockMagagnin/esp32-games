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
#include "emu/Pack.h"
#include "play/Player.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameListScreen.h"
#include "screens/SettingsScreen.h"
#include "ui/Themes.h"

// Linked in by tools/pack.py
extern "C" const uint8_t gb_pack_start[];
extern "C" const uint8_t gb_pack_end[];

// The same carrier board as the idle game: a D-pad with Confirm in the middle
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

// The emulator reads the buttons once a frame: the library's 50 ms would swallow a quick tap
static const unsigned long GAME_DEBOUNCE_MS = 15;

// Both editions, the lite and the max, keep their saves here: the same games under the same keys
const char *const NAMESPACE = "gameboy";

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, GAME_DEBOUNCE_MS);
Button *const KEY_BUTTONS[] = {&upButton, &downButton, &leftButton, &rightButton, &confirmButton};

Pack pack;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

void resetAll();
void applySettings();
void playGame(uint16_t index);
void backToList();
ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every game's save and the", "settings will be erased.",
                           "Resetting..."},
                          Theme::BAD, resetAll, screenManager, sound);
GameListScreen listScreen(pack, NAMESPACE, playGame, sound);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, screenManager, sound, resetScreen);
Player player(tft, pack, NAMESPACE, sound, backToList, returnToLauncher);

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

void resetAll()
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.clear();
        prefs.end();
    }
    ESP.restart();
}

void playGame(uint16_t index)
{
    sound.playStart();
    if (!player.start(index))
    {
        Serial.printf("Game %u refused\n", (unsigned)index);
        screenManager.repaint();
    }
}

void backToList()
{
    screenManager.setScreen(screenManager.getCurrentIndex());
}

// Holding a button repeats it while the list scrolls
static const unsigned long REPEAT_DELAY_MS = 350;
static const unsigned long REPEAT_STEP_MS = 80;
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

#ifdef DEV_MODE
// Serial commands for testing: `screen <i>` switches screen, `play <i>` starts a game, `shot`
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
        if (strncmp(line, "screen ", 7) == 0)
        {
            screenManager.setScreen(atoi(line + 7));
        }
        else if (strncmp(line, "play ", 5) == 0)
        {
            playGame((uint16_t)atoi(line + 5));
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (line[0] != '\0')
        {
            Serial.println("screen <i> | play <i> | shot");
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
    // the way out should a game fail before its Settings
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }
    // Holding Up and Down while it powers on erases every save and the settings
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        Preferences prefs;
        if (prefs.begin(NAMESPACE, false))
        {
            prefs.clear();
            prefs.end();
        }
        Serial.println("Saves erased");
    }

    loadSettings(NAMESPACE, settings);
    applySettings();

    if (!pack.open(gb_pack_start, (size_t)(gb_pack_end - gb_pack_start)))
    {
        Serial.println("The game pack is missing or broken");
    }
    Serial.printf("%u games\n", (unsigned)pack.count());

    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(Theme::BG);
    if (!player.begin())
    {
        Serial.println("No DMA: drawing without it");
    }
    backlight.begin();

    // screenshot.py's --screen counts on this order
    screenManager.addScreen(&listScreen, "Games");
    screenManager.addScreen(&settingsScreen, "Settings");
    backlight.setFull();
}

void loop()
{
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

    if (player.active())
    {
        Pad pad = {upButton.isDown(), downButton.isDown(), leftButton.isDown(), rightButton.isDown(),
                   confirmButton.isDown(), up, down, confirm};
        player.loop(pad);
        return;
    }

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
    }

    screenManager.update(now);
}
