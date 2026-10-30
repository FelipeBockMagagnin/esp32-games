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
#include "nan/Format.h"
#include "nan/Game.h"
#include "screens/AdventureScreen.h"
#include "screens/Allocation.h"
#include "screens/AugmentScreen.h"
#include "screens/BossScreen.h"
#include "screens/DialogScreen.h"
#include "screens/InventoryScreen.h"
#include "screens/MinerScreen.h"
#include "screens/RebirthScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SplashScreen.h"
#include "screens/TrainingScreen.h"
#include "screens/XpScreen.h"
#include "ui/Themes.h"

const int RIGHT_BUTTON_PIN = 23;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 19;
const int DOWN_BUTTON_PIN = 22;
const int LEFT_BUTTON_PIN = 26;
const int BUZZER_PIN = 32;

static_assert(sizeof(Game::Snapshot) <= SlotStore::MAX_SIZE, "the save must fit one SlotStore record");

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

SlotStore store("nanidle");
Game game(hardwareRandom);
Allocation allocation;
// Windows XP until the player picks another
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 1};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);

void applySettings();
void saveNow();
void toMenu();
void resetAll();
void rebirthNow();

DialogScreen dialog(screenManager, sound);
SplashScreen splashScreen(screenManager, sound);
TrainingScreen trainingScreen(game, allocation, sound);
BossScreen bossScreen(game, sound);
XpScreen xpScreen(game, sound);
AdventureScreen adventureScreen(game, sound);
InventoryScreen inventoryScreen(game, sound);
MinerScreen minerScreen(game, sound);
AugmentScreen augmentScreen(game, allocation, sound);
RebirthScreen rebirthScreen(game, sound, dialog, rebirthNow);
SettingsScreen settingsScreen(settings, applySettings, toMenu, resetAll, screenManager, sound, dialog);

// The screen each feature opens, in Feature order; the adventure opens the inventory with it
Screen *const FEATURE_SCREENS[FEATURE_COUNT] = {&bossScreen,   &xpScreen,      &adventureScreen,
                                                &minerScreen,  &augmentScreen, &rebirthScreen};
static const char *const FEATURE_NAMES[FEATURE_COUNT] = {"Lutar com Boss", "Gastar XP", "Aventura e Itens",
                                                         "Mineradores", "Aprimoramentos", "Renascer"};
// The rotation, in addScreen() order: screenshot.py's --screen counts on it
enum ScreenIndex : uint8_t
{
    SCREEN_TRAINING,
    SCREEN_BOSS,
    SCREEN_XP,
    SCREEN_ADVENTURE,
    SCREEN_INVENTORY,
    SCREEN_MINERS,
    SCREEN_AUGMENTS,
    SCREEN_REBIRTH,
    SCREEN_SETTINGS
};
static const uint8_t FEATURE_INDEX[FEATURE_COUNT] = {SCREEN_BOSS,   SCREEN_XP,       SCREEN_ADVENTURE,
                                                     SCREEN_MINERS, SCREEN_AUGMENTS, SCREEN_REBIRTH};

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

// ---- Saving ---------------------------------------------------------------------------------

// A minute of idle progress is cheap to lose; a purchase, a drop or a boss brings the next
// save forward to a few seconds. The interval is NVS wear, so keep it.
static const unsigned long SAVE_EVERY_MS = 60000;
static const unsigned long MILESTONE_SAVE_MS = 5000;
Game::Snapshot saveBuffer;
Game::Snapshot lastSaved;
unsigned long nextSaveAt = 0;
uint32_t savedMilestones = 0;

void saveNow()
{
    game.capture(saveBuffer);
    if (memcmp(&saveBuffer, &lastSaved, sizeof(saveBuffer)) == 0)
    {
        return;
    }
    if (store.save(&saveBuffer, sizeof(saveBuffer)))
    {
        lastSaved = saveBuffer;
    }
    else
    {
        Serial.println("Save failed");
    }
}

void autoSave(unsigned long now)
{
    if (game.milestones() != savedMilestones)
    {
        savedMilestones = game.milestones();
        if ((long)(nextSaveAt - (now + MILESTONE_SAVE_MS)) > 0)
        {
            nextSaveAt = now + MILESTONE_SAVE_MS;
        }
    }
    if ((long)(now - nextSaveAt) >= 0)
    {
        saveNow();
        nextSaveAt = now + SAVE_EVERY_MS;
    }
}

void toMenu()
{
    saveNow();
    returnToLauncher();
}

void resetAll()
{
    store.erase();
    ESP.restart();
}

void rebirthNow()
{
    game.rebirth();
    saveNow();
    sound.playRebirth();
    screenManager.setScreen(SCREEN_TRAINING);
}

// Holding Up and Down while the board powers on wipes the save and settings
void loadOrReset()
{
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
        return;
    }
    Game::Snapshot loaded;
    if (store.load(&loaded, sizeof(loaded)) && game.restore(loaded))
    {
        lastSaved = loaded;
        Serial.println("Save loaded");
    }
}

// ---- Unlocks and notices --------------------------------------------------------------------

uint32_t seenUnlocks = 0;
uint32_t seenBossKills = 0;
uint32_t seenDrops = 0;
uint8_t unlockedScreen = SCREEN_TRAINING;

void applyUnlocks()
{
    for (uint8_t f = 0; f < FEATURE_COUNT; f++)
    {
        screenManager.setLocked(FEATURE_SCREENS[f], !game.isUnlocked((Feature)f));
    }
    screenManager.setLocked(&inventoryScreen, !game.isUnlocked(FEAT_ADVENTURE));
}

void openUnlocked()
{
    screenManager.setScreen(unlockedScreen);
}

void checkEvents(unsigned long now)
{
    if (game.unlockEvents() != seenUnlocks && !screenManager.hasModal())
    {
        seenUnlocks = game.unlockEvents();
        applyUnlocks();
        Feature f = game.lastUnlock();
        unlockedScreen = FEATURE_INDEX[f];
        const char *lines[DialogScreen::LINES] = {"Novo recurso liberado:", FEATURE_NAMES[f], "",
                                                  "Ele ja esta na barra."};
        sound.playNotice();
        dialog.open("NaN Idle", DialogIcon::INFO, lines, "OK", nullptr, openUnlocked);
    }

    Screen *current = screenManager.getCurrentScreen();
    char buf[24];
    if (game.bossKills() != seenBossKills)
    {
        seenBossKills = game.bossKills();
        if (current != &bossScreen)
        {
            snprintf(buf, sizeof(buf), "Boss %u derrotado!", (unsigned)game.bossesBeaten());
            current->applyNotice(buf, now + 3000);
            sound.playBoss();
        }
    }
    if (game.drops() != seenDrops)
    {
        seenDrops = game.drops();
        if (current != &adventureScreen)
        {
            snprintf(buf, sizeof(buf), "Drop: %s", ITEMS[game.lastDrop()].name);
            current->applyNotice(buf, now + 2500);
        }
    }

    // The tray clock: hours and minutes played
    uint32_t minutes = game.playSeconds() / 60;
    snprintf(buf, sizeof(buf), "%lu:%02lu", (unsigned long)(minutes / 60 % 1000), (unsigned long)(minutes % 60));
    Screen::taskbar().setClock(buf);
}

// ---- The game clock -------------------------------------------------------------------------

static const unsigned long TICK_MS = 100;
// A loop stalled longer than this (a save, a flash write) drops the rest instead of bursting
static const uint8_t MAX_CATCH_UP = 10;
unsigned long lastTick = 0;

void tickGame(unsigned long now)
{
    uint8_t steps = 0;
    while (now - lastTick >= TICK_MS)
    {
        if (++steps > MAX_CATCH_UP)
        {
            lastTick = now;
            break;
        }
        lastTick += TICK_MS;
        game.tick(TICK_MS / 1000.0);
    }
}

// ---- Input ----------------------------------------------------------------------------------

static const unsigned long REPEAT_DELAY_MS = 350;
static const unsigned long REPEAT_STEP_MS = 90;
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

#ifdef DEV_MODE
// Serial commands for testing: `screen <i>`, `shot` (tools/screenshot.py), and cheats
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
        Game::Snapshot snap;
        game.capture(snap);
        if (strncmp(line, "screen ", 7) == 0)
        {
            screenManager.setScreen(atoi(line + 7));
        }
        else if (strcmp(line, "shot") == 0)
        {
            sendScreen();
        }
        else if (strncmp(line, "xp ", 3) == 0 || strncmp(line, "gold ", 5) == 0 || strncmp(line, "boss ", 5) == 0 ||
                 strncmp(line, "level ", 6) == 0)
        {
            double value = atof(strchr(line, ' ') + 1);
            if (line[0] == 'x')
            {
                snap.xp = value;
            }
            else if (line[0] == 'g')
            {
                snap.gold = value;
            }
            else if (line[0] == 'b')
            {
                snap.bossesBeaten = (uint16_t)value;
                snap.bossHp = 0;
            }
            else
            {
                for (uint8_t i = 0; i < SKILL_COUNT; i++)
                {
                    snap.skillLevel[i] = (uint32_t)value;
                }
            }
            game.restore(snap);
            seenUnlocks = game.unlockEvents();
            seenBossKills = game.bossKills();
            seenDrops = game.drops();
        }
        else if (line[0] != '\0')
        {
            Serial.println("screen <i> | shot | xp <n> | gold <n> | boss <n> | level <n>");
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
    // the way out should the game fail before its Settings
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }

    loadOrReset();
    loadSettings(store.getNamespace(), settings);
    applySettings();
    seenUnlocks = game.unlockEvents();
    seenBossKills = game.bossKills();
    seenDrops = game.drops();

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(0x0000);
    backlight.begin();

    screenManager.addScreen(&trainingScreen, "Treino");
    screenManager.addScreen(&bossScreen, "Boss", true);
    screenManager.addScreen(&xpScreen, "XP", true);
    screenManager.addScreen(&adventureScreen, "Aventura", true);
    screenManager.addScreen(&inventoryScreen, "Itens", true);
    screenManager.addScreen(&minerScreen, "Minerar", true);
    screenManager.addScreen(&augmentScreen, "Aprimorar", true);
    screenManager.addScreen(&rebirthScreen, "Renascer", true);
    screenManager.addScreen(&settingsScreen, "Config");
    applyUnlocks();
    screenManager.openModal(&splashScreen);
    backlight.setFull();

    unsigned long now = millis();
    lastTick = now;
    nextSaveAt = now + SAVE_EVERY_MS;
    savedMilestones = game.milestones();
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
    }
    else if (right)
    {
        if (!screenManager.handleRightPress())
        {
            sound.playMenu();
        }
    }
    else if (confirm)
    {
        screenManager.handleConfirmPress();
        startRepeat(now, Key::CONFIRM);
    }
    checkRepeat(now);

    tickGame(now);
    checkEvents(now);
    autoSave(now);
    screenManager.update(now);
}
