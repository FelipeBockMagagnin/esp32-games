#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Backlight.h"
#include "ButtonManager.h"
#include "Launcher.h"
#include "Settings.h"
#include "SlotStore.h"
#include "app/Playlist.h"
#include "app/Save.h"
#include "app/Transport.h"
#include "audio/Player.h"
#include "core/ScreenManager.h"
#include "screens/CreditsScreen.h"
#include "screens/LibraryScreen.h"
#include "screens/PlayerScreen.h"
#include "screens/SettingsScreen.h"
#include "ui/Themes.h"
#include "ui/Turntable.h"

// The same carrier board as the idle game: a D-pad with Confirm in the middle
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

static const unsigned long REPEAT_DELAY_MS = 400;
static const unsigned long REPEAT_EVERY_MS = 110;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button upButton = Button(UP_BUTTON_PIN);
Button downButton = Button(DOWN_BUTTON_PIN);
Button leftButton = Button(LEFT_BUTTON_PIN);

uint32_t hardwareRandom()
{
    return esp_random();
}

SlotStore store("vitrola");
SaveData save;
Settings settings = {0, DIM_DEFAULT, 0};
Backlight backlight(TFT_BL);
Player player;
Playlist playlist(hardwareRandom);
Turntable turntable;
ScreenManager screenManager(tft);

void saveState();
void applySettings();
Transport transport(player, playlist, saveState);
// The modals name each other, so they are built in this order and wired through references
extern SettingsScreen settingsScreen;
CreditsScreen creditsScreen(screenManager, (Screen &)settingsScreen);
SettingsScreen settingsScreen(settings, applySettings, returnToLauncher, transport, screenManager, creditsScreen);
LibraryScreen libraryScreen(transport, screenManager, settingsScreen);
PlayerScreen playerScreen(transport, turntable, screenManager, libraryScreen);

void saveState()
{
    save.track = playlist.current;
    save.mode = (uint8_t)playlist.mode;
    if (!writeSave(store, save))
    {
        Serial.println("Save failed");
    }
}

void applySettings()
{
    if (settings.dimAfter >= DIM_OPTION_COUNT)
    {
        settings.dimAfter = DIM_DEFAULT;
    }
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
    saveSettings(store.getNamespace(), settings);
}

#ifdef DEV_MODE
// Serial commands for testing: `shot` sends the panel to tools/screenshot.py, `play <i>` a track
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
        if (strncmp(line, "play ", 5) == 0)
        {
            transport.play(atoi(line + 5));
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (strcmp(line, "heap") == 0)
        {
            Serial.printf("free %u, largest %u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        }
        else if (line[0] != '\0')
        {
            Serial.println("play <i> | shot | heap");
        }
    }
}
#endif

// Up and Down repeat while held, for long lists
static void repeatHeld(unsigned long now, Button &button, Key key)
{
    static unsigned long since[2] = {0, 0};
    static unsigned long nextAt[2] = {0, 0};
    uint8_t i = key == Key::UP ? 0 : 1;
    if (!button.isDown())
    {
        since[i] = 0;
        return;
    }
    if (since[i] == 0)
    {
        since[i] = now;
        nextAt[i] = now + REPEAT_DELAY_MS;
        return;
    }
    if ((long)(now - nextAt[i]) >= 0)
    {
        nextAt[i] = now + REPEAT_EVERY_MS;
        screenManager.handleRepeat(key);
    }
}

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
    // the way out should the app fail before its Settings
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }

    defaultSave(save);
    readSave(store, save);
    playlist.current = save.track;
    playlist.mode = (Mode)save.mode;
    loadSettings(store.getNamespace(), settings);
    applySettings();

    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(Theme::BG);
    backlight.begin();
    turntable.begin();
    player.begin(BUZZER_PIN);

    screenManager.addScreen(&playerScreen, "Vitrola");
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
    repeatHeld(now, upButton, Key::UP);
    repeatHeld(now, downButton, Key::DOWN);

    transport.update();
    screenManager.update(now);
}
