#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "managers/ButtonManager.h"
#include "managers/ScreenManager.h"
#include "managers/SoundManager.h"
#include "app/Clock.h"
#include "app/FocusTimer.h"
#include "app/FocusStats.h"
#include "app/Storage.h"
#include "screens/ClockScreen.h"
#include "screens/FocusScreen.h"
#include "screens/StatsScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/ConfirmScreen.h"
#include "ui/Themes.h"
#include "debug/DevConsole.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

// The same board as the idle game: a D-pad with Confirm in the middle
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

Clock wallClock;
FocusTimer timer;
FocusStats stats;
SoundManager sound(BUZZER_PIN);
ScreenManager screenManager(tft);

ClockScreen clockScreen(wallClock, timer);
FocusScreen focusScreen(timer, sound);
StatsScreen statsScreen(stats, wallClock);

void resetStats();
void applySettings();
ConfirmScreen resetScreen({"Reset", "Reset stats?", "Every focus count is erased.", "Settings stay.",
                           "Erase the stats and restart", "Resetting..."},
                          Theme::BAD, resetStats, screenManager, sound);
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0, FOCUS_DEFAULT, BREAK_DEFAULT, LONG_BREAK_DEFAULT, 0};
SettingsScreen settingsScreen(settings, applySettings, screenManager, sound, resetScreen);

#ifdef DEV_MODE
DevConsole devConsole(timer, tft, screenManager);
#endif

// tone() drives LEDC channel 0, and channel 1 shares its timer
static const uint8_t BACKLIGHT_CHANNEL = 4;
static const uint8_t BACKLIGHT_FULL = 255;
static const uint8_t BACKLIGHT_DIM = 20;
unsigned long lastInputAt = 0;
bool dimmed = false;

static const unsigned long NOTICE_MS = 8000;
uint32_t seenPhaseEnds = 0;
const char *noticeCaption = nullptr;
const char *noticeText = nullptr;
Color noticeAccent = Theme::GOLD;
unsigned long noticeUntil = 0;

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
    if (settings.focusMin >= FOCUS_OPTION_COUNT)
    {
        settings.focusMin = FOCUS_DEFAULT;
    }
    if (settings.breakMin >= BREAK_OPTION_COUNT)
    {
        settings.breakMin = BREAK_DEFAULT;
    }
    if (settings.longBreakMin >= LONG_BREAK_OPTION_COUNT)
    {
        settings.longBreakMin = LONG_BREAK_DEFAULT;
    }
    settings.autoStart = settings.autoStart ? 1 : 0;

    Theme::setActive(settings.theme);
    sound.setMode((SoundMode)settings.sound);
    timer.setDurations(FOCUS_OPTIONS[settings.focusMin], BREAK_OPTIONS[settings.breakMin],
                       LONG_BREAK_OPTIONS[settings.longBreakMin]);
    timer.setAutoStart(settings.autoStart);
    saveSettings(settings);
}

void resetStats()
{
    eraseStats();
    ESP.restart();
}

void wake(unsigned long now)
{
    lastInputAt = now;
    if (dimmed)
    {
        dimmed = false;
        ledcWrite(BACKLIGHT_CHANNEL, BACKLIGHT_FULL);
    }
}

// True when the press only woke the screen or silenced the alarm, so it must go no further
bool absorbInput(unsigned long now, bool pressed, bool held)
{
    if (pressed && sound.isRinging())
    {
        sound.stopAlarm();
        noticeUntil = now;
        wake(now);
        return true;
    }
    if (pressed || held)
    {
        lastInputAt = now;
    }
    if (dimmed && pressed)
    {
        wake(now);
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

static const unsigned long SETTINGS_HOLD_MS = 1500;
unsigned long settingsHoldStart = 0;
bool settingsHolding = false;
bool settingsHoldFired = false; // Latched until Left is released, so one hold opens it once

void checkSettingsGesture(unsigned long now)
{
    if (!leftButton.isDown())
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

// A phase that ran out rings, lights the screen, shows the Focus screen and, for a focus,
// counts it. Stats are only written here, a few times an hour at most.
void checkPhaseEnd(unsigned long now)
{
    if (timer.getPhaseEndEvents() == seenPhaseEnds)
    {
        return;
    }
    seenPhaseEnds = timer.getPhaseEndEvents();
    bool focusEnded = timer.getEndedPhase() == Phase::FOCUS;
    if (focusEnded)
    {
        stats.record(timer.getFocusMinutes(), wallClock.today());
        FocusStats::Snapshot snapshot;
        stats.save(snapshot);
        if (!saveStats(snapshot))
        {
            Serial.println("Stats save failed");
        }
    }

    sound.startAlarm(focusEnded, now);
    wake(now);
    noticeCaption = "TIME'S UP";
    noticeText = focusEnded ? "Take a break" : "Back to focus";
    noticeAccent = focusEnded ? Theme::GOOD : Theme::BAD;
    noticeUntil = now + NOTICE_MS;
    screenManager.setScreen(1);
}

void setup()
{
    Serial.begin(115200);

    rightButton.setup();
    confirmButton.setup();
    upButton.setup();
    downButton.setup();
    leftButton.setup();

    loadSettings(settings);
    applySettings();
    FocusStats::Snapshot snapshot;
    if (loadStats(snapshot))
    {
        stats.load(snapshot);
    }

    wallClock.begin(WIFI_SSID, WIFI_PASSWORD, TIMEZONE);

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::BG);
    ledcSetup(BACKLIGHT_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, BACKLIGHT_CHANNEL);

    // This order is the navigation order; checkPhaseEnd() and screenshot.py count on it
    screenManager.addScreen(&clockScreen, "Clock");
    screenManager.addScreen(&focusScreen, "Focus");
    screenManager.addScreen(&statsScreen, "Stats");
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
    if (absorbInput(now, right || confirm || up || down || left, held))
    {
        right = confirm = up = down = left = false;
    }

    checkSettingsGesture(now);
#ifdef DEV_MODE
    devConsole.update();
#endif

    if (right && !screenManager.hasModal())
    {
        sound.playMenu();
        screenManager.nextScreen();
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

    wallClock.update(now);
    timer.update(now);
    checkPhaseEnd(now);
    sound.update(now);

    Screen *current = screenManager.getCurrentScreen();
    if (current != nullptr)
    {
        bool showing = (long)(now - noticeUntil) < 0;
        current->applyNotice(noticeCaption, showing ? noticeText : nullptr, noticeAccent);
    }

    screenManager.update(now);
}
