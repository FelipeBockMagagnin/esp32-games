#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "audio/Songs.h"
#include "core/Backlight.h"
#include "core/ButtonManager.h"
#include "core/ScreenManager.h"
#include "core/Settings.h"
#include "core/SlotStore.h"
#include "core/SoundManager.h"
#include "jack/Profile.h"
#include "jack/Round.h"
#include "jack/Save.h"
#include "screens/ConfirmScreen.h"
#include "screens/GameOverScreen.h"
#include "screens/PlayScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/ShopScreen.h"
#include "ui/Themes.h"

// The same carrier board as the idle game: a D-pad with Confirm in the middle
const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

// A turn is felt at once, so the buttons settle faster than the engine's 50 ms
static const unsigned long DEBOUNCE_MS = 15;

TFT_eSPI tft = TFT_eSPI();
Button rightButton = Button(RIGHT_BUTTON_PIN, DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, DEBOUNCE_MS);

SlotStore store("jackhammer");
Profile profile;
Round currentRound;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound;
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

void resetGame();
void applySettings();
void shopChanged();
void died();
ConfirmScreen resetScreen({"Reset", "Reset game?", "Coins, best and purchases", "will be erased.",
                           "Erase the save and restart", "Resetting..."},
                          Theme::BAD, resetGame, screenManager, sound);
SettingsScreen settingsScreen(settings, applySettings, screenManager, sound, resetScreen);
ShopScreen shopScreen(profile, screenManager, sound, shopChanged);
PlayScreen playScreen(currentRound, profile, sound, screenManager, shopScreen, settingsScreen, died);
GameOverScreen gameOverScreen(currentRound, profile, playScreen, screenManager, sound);

void saveNow()
{
    if (!writeSave(store, profile))
    {
        Serial.println("Save failed");
    }
}

void applySong()
{
    const Song &song = SONG_TRACKS[profile.song];
    sound.setSong(song.notes, song.count);
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

void shopChanged()
{
    applySong();
    playScreen.styleChanged();
    saveNow();
}

void died()
{
    gameOverScreen.setNewBest(bankRound(profile, currentRound.getPoints(), currentRound.takeUnbanked()));
    saveNow();
    screenManager.openModal(&gameOverScreen);
}

// Holding Up and Down while the board powers on wipes the save
void loadOrReset()
{
    newProfile(profile);
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readSave(store, profile))
    {
        Serial.println("Save loaded");
    }
}

#ifdef DEV_MODE
// Serial commands for testing: `coins <n>` sets the coins, `shot` sends the panel to
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
        if (strncmp(line, "coins ", 6) == 0)
        {
            profile.coins = (uint32_t)strtoul(line + 6, nullptr, 10);
            saveNow();
            screenManager.repaint();
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (line[0] != '\0')
        {
            Serial.println("coins <n> | shot");
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
    sound.begin(BUZZER_PIN);
    loadSettings(store.getNamespace(), settings);
    applySettings();
    applySong();

    tft.init();
    tft.setRotation(2);
    tft.initDMA();
    tft.fillScreen(Theme::BG);
    backlight.begin();

    playScreen.toTitle();
    screenManager.addScreen(&playScreen, "Play");
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

    // Every press is handled, not one per loop: a quick Left then Right in a round is two turns
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
}
