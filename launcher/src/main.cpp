#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_ota_ops.h>
#include "Catalog.h"
#include "Menu.h"
#include "Backlight.h"
#include "ButtonManager.h"
#include "pitches.h"

// The same carrier board as the idle game: a D-pad with Confirm in the middle
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

static const char *const NAMESPACE = "launcher";
static const char *const KEY_LAST = "last";
static const unsigned long DIM_AFTER_MS = 120000;
// Lets the chime finish before the restart cuts it off
static const unsigned long LAUNCH_DELAY_MS = 400;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN);
Button confirmButton = Button(CONFIRM_BUTTON_PIN);
Button upButton = Button(UP_BUTTON_PIN);
Button downButton = Button(DOWN_BUTTON_PIN);
Button leftButton = Button(LEFT_BUTTON_PIN);
Backlight backlight(TFT_BL);
Catalog catalog;
Menu menu(tft, catalog);

bool launching = false;
unsigned long launchAt = 0;

int lastPlayed()
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, true))
    {
        return -1;
    }
    char label[17] = {};
    prefs.getString(KEY_LAST, label, sizeof(label));
    prefs.end();
    return catalog.indexOf(label);
}

void startLaunch(unsigned long now)
{
    const Game &g = catalog.game(menu.getSelected());
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.putString(KEY_LAST, g.app->label);
        prefs.end();
    }
    menu.showLaunching();
    tone(BUZZER_PIN, NOTE_C6, 70);
    tone(BUZZER_PIN, NOTE_G6, 70);
    tone(BUZZER_PIN, NOTE_C7, 160);
    launching = true;
    launchAt = now + LAUNCH_DELAY_MS;
}

// From here on the board boots straight into the game, until it asks to come back
void launch()
{
    const Game &g = catalog.game(menu.getSelected());
    esp_err_t err = esp_ota_set_boot_partition(g.app);
    if (err != ESP_OK)
    {
        Serial.printf("Cannot boot %s: %s\n", g.app->label, esp_err_to_name(err));
        launching = false;
        menu.begin(menu.getSelected());
        return;
    }
    esp_restart();
}

#ifdef DEV_MODE
// `shot` sends the panel to tools/screenshot.py; `play <n>` starts the nth game, as Confirm would
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
    static char line[16];
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
        if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (strncmp(line, "play ", 5) == 0 && !launching && atoi(line + 5) < catalog.count())
        {
            menu.begin(atoi(line + 5));
            startLaunch(millis());
        }
        else if (line[0] != '\0')
        {
            Serial.println("shot | play <n>");
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

    catalog.load();
    Serial.printf("%u games\n", catalog.count());

    tft.init();
    tft.setRotation(2);
    backlight.begin();
    menu.begin(lastPlayed());
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
    if (backlight.update(now, right || confirm || up || down || left, held, DIM_AFTER_MS))
    {
        right = confirm = up = down = left = false;
    }
#ifdef DEV_MODE
    devConsole();
#endif

    if (launching)
    {
        if ((long)(now - launchAt) >= 0)
        {
            launch();
        }
        return;
    }
    if (catalog.count() == 0)
    {
        return;
    }
    if (left || up)
    {
        menu.step(-1);
        tone(BUZZER_PIN, NOTE_GS7, 40);
    }
    else if (right || down)
    {
        menu.step(1);
        tone(BUZZER_PIN, NOTE_GS7, 40);
    }
    else if (confirm)
    {
        startLaunch(now);
    }
}
