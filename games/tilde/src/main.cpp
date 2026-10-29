#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Backlight.h"
#include "ButtonManager.h"
#include "core/ScreenManager.h"
#include "Settings.h"
#include "SlotStore.h"
#include "core/SoundManager.h"
#include "pet/Pet.h"
#include "pet/Album.h"
#include "pet/Save.h"
#include "art/Rig.h"
#include "gfx/Display.h"
#include "gfx/Themes.h"
#include "screens/AlbumScreen.h"
#include "screens/ConfirmScreen.h"
#include "screens/HatchScreen.h"
#include "screens/HomeScreen.h"
#include "screens/PlayScreen.h"
#include "screens/SettingsScreen.h"
#include "screens/SplashScreen.h"
#include "screens/StatsScreen.h"

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

Display display(tft);
SlotStore store("tilde");
Pet pet;
Album album;
SaveBlob saveBuffer;
Settings settings = {(uint8_t)SoundMode::ON, DIM_DEFAULT, 0};
SoundManager sound(BUZZER_PIN);
Backlight backlight(TFT_BL);
ScreenManager screenManager(tft);
AutoSaver saver(store, pet, album);
Game game = {pet, album, sound, screenManager, saver};

void openPlay();
void applySettings();
void resetGame();
void passOnEgg();
HatchScreen hatchScreen(display, game);
HomeScreen homeScreen(display, game, hatchScreen, openPlay);
StatsScreen statsScreen(display, game);
PlayScreen playScreen(display, game);
AlbumScreen albumScreen(display, game);
SplashScreen splashScreen(display, game);
ConfirmScreen resetScreen(display, game,
                          {"Reset", "Reset the game?", "The pet and the Album", "will be erased.", "Erase and restart"},
                          ALERT, resetGame);
ConfirmScreen farewellScreen(display, game,
                             {"Pass on", "Pass on an egg?", "Your pet keeps a page in the", "Album, and a new egg arrives.",
                              "Yes, a new egg"},
                             BLUSH, passOnEgg);
SettingsScreen settingsScreen(display, game, settings, applySettings, farewellScreen, resetScreen);

static const int PLAY_SCREEN_INDEX = 2;

void pollButtons()
{
    rightButton.loop();
    confirmButton.loop();
    upButton.loop();
    downButton.loop();
    leftButton.loop();
}

void openPlay()
{
    screenManager.setScreen(PLAY_SCREEN_INDEX);
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

void passOnEgg()
{
    album.farewell(pet);
    saver.saveNow();
    screenManager.closeModal();
    screenManager.setScreen(0);
    homeScreen.restart(millis());
    screenManager.repaint();
}

// A pet's call is a sound at most this often; the header notice shows every time
static const unsigned long CALL_SOUND_EVERY_MS = 600000;
unsigned long nextTickAt = 0;
unsigned long callSoundAt = 0;
bool callSounded = false;

void call(const char *what, unsigned long now)
{
    char text[48];
    snprintf(text, sizeof(text), "%s %s", pet.getName(), what);
    Screen::notify(text, ALERT, now, 6000);
    if (!callSounded || now - callSoundAt >= CALL_SOUND_EVERY_MS)
    {
        callSounded = true;
        callSoundAt = now;
        sound.playCall(FORM_RIG[pet.getForm()].voice);
    }
}

void handleEvents(uint16_t events, unsigned long now)
{
    if (events == 0)
    {
        return;
    }
    saver.soon(now);
    char text[48];
    if (events & EV_HATCHED)
    {
        album.see(pet.getForm());
        if (!screenManager.hasModal())
        {
            screenManager.setScreen(0);
        }
    }
    if (events & EV_EVOLVED)
    {
        album.see(pet.getForm());
        sound.playEvolve();
        snprintf(text, sizeof(text), "%s grew into a %s!", pet.getName(), FORMS[pet.getForm()].name);
        Screen::notify(text, GOLD, now, 6000);
    }
    if (events & EV_SICK)
    {
        call("feels unwell", now);
    }
    else if (events & EV_CALL)
    {
        call(NEEDS[pet.lastCall()].wants, now);
    }
    if (events & EV_WOKE)
    {
        sound.playVoice(FORM_RIG[pet.getForm()].voice);
        snprintf(text, sizeof(text), "%s woke up!", pet.getName());
        Screen::notify(text, GOLD, now, 3000);
    }
}

void tickPet(unsigned long now)
{
    if ((long)(now - nextTickAt) < 0)
    {
        return;
    }
    nextTickAt = now + 1000;
    pet.tick();
    handleEvents(pet.takeEvents(), now);
}

// Holding a button repeats it at a steady pace
static const unsigned long REPEAT_DELAY_MS = 350;
static const unsigned long REPEAT_STEP_MS = 120;
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

// Hold Left for Settings and Up + Down for the reset question, except where the arrows play.
// The first touch still reaches the screen as an ordinary press.
struct Hold
{
    unsigned long since = 0;
    bool holding = false;
    bool fired = false;

    bool check(bool down, unsigned long now, unsigned long ms)
    {
        if (!down)
        {
            holding = fired = false;
            return false;
        }
        if (!holding)
        {
            holding = true;
            since = now;
        }
        if (fired || now - since < ms)
        {
            return false;
        }
        fired = true;
        return true;
    }
};
Hold settingsHold;
Hold resetHold;

void checkGestures(unsigned long now)
{
    Screen *current = screenManager.getCurrentScreen();
    bool open = !screenManager.hasModal() && current != nullptr && current->allowsGestures();
    bool settingsDown = leftButton.isDown() && !upButton.isDown() && !downButton.isDown();
    if (settingsHold.check(settingsDown && open, now, 1500))
    {
        repeating = false;
        sound.playMenu();
        screenManager.openModal(&settingsScreen);
    }
    if (resetHold.check(upButton.isDown() && downButton.isDown() && open, now, 3000))
    {
        repeating = false;
        sound.playMenu();
        screenManager.openModal(&resetScreen);
    }
}

// Holding Up and Down while the board powers on wipes the save
void loadOrReset()
{
    album.clear();
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        store.erase();
        Serial.println("Save erased");
    }
    else if (readSave(store, saveBuffer))
    {
        pet.load(saveBuffer.pet);
        album = saveBuffer.album;
        Serial.println("Save loaded");
    }
    pet.reseed(esp_random());
}

#ifdef DEV_MODE
// Serial commands for testing: `age <s>` lives that many seconds at once, `need <i> <v>`,
// `bond <n>`, `form <i>`, `sick`, `fluff`, `screen <i>`, `theme <i>`, `fps` prints the last
// frame's cost, and `shot` sends the panel to tools/screenshot.py
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

void devCommand(const char *line)
{
    unsigned long now = millis();
    int a = 0, b = 0;
    Pet::Snapshot &raw = pet.raw();
    if (sscanf(line, "age %d", &a) == 1 && a > 0)
    {
        uint16_t events = 0;
        for (int i = 0; i < a; i++)
        {
            pet.tick();
            events |= pet.takeEvents();
        }
        handleEvents(events, now);
    }
    else if (sscanf(line, "need %d %d", &a, &b) == 2 && a >= 0 && a < NEED_COUNT)
    {
        raw.needs[a] = (uint8_t)(b < 0 ? 0 : b > NEED_MAX ? NEED_MAX : b);
    }
    else if (sscanf(line, "bond %d", &a) == 1)
    {
        raw.bond = (uint16_t)(a < 0 ? 0 : a > BOND_MAX ? BOND_MAX : a);
    }
    else if (sscanf(line, "form %d", &a) == 1 && a >= 0 && a < FORM_COUNT)
    {
        raw.form = (uint8_t)a;
        raw.stageSecs = 0;
        album.see((FormId)a);
        homeScreen.restart(now);
    }
    else if (strcmp(line, "sick") == 0)
    {
        raw.flags |= 1 << 2;
    }
    else if (strcmp(line, "fluff") == 0 && raw.fluff < MAX_FLUFF)
    {
        raw.fluffSpot[raw.fluff++] = (uint8_t)random(ROOM_SPOTS);
    }
    else if (sscanf(line, "screen %d", &a) == 1)
    {
        screenManager.closeModal();
        screenManager.setScreen(a);
        return;
    }
    else if (sscanf(line, "theme %d", &a) == 1)
    {
        settings.theme = (uint8_t)a;
        applySettings();
    }
    else if (strcmp(line, "fps") == 0)
    {
        Serial.printf("frame %lu us, drawing %lu us, %u of %u segments sent\nsegment us:",
                      (unsigned long)display.getFrameMicros(), (unsigned long)display.getDrawMicros(),
                      (unsigned)display.getSegmentsSent(), (unsigned)display.getSegmentCount());
        for (uint8_t i = 0; i < display.getSegmentCount(); i++)
        {
            Serial.printf(" %lu", (unsigned long)display.getSegmentMicros(i));
        }
        Serial.println();
        return;
    }
    else if (strcmp(line, "shot") == 0)
    {
        sendScreen();
        return;
    }
    else if (line[0] != '\0')
    {
        Serial.println("age <s> | need <i> <v> | bond <n> | form <i> | sick | fluff | screen <i> | theme <i> | fps "
                       "| shot");
        return;
    }
    screenManager.repaint();
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
        devCommand(line);
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
    saver.begin(millis());

    tft.init();
    tft.setRotation(2);
    tft.fillScreen(Theme::rgb(BG));
    display.begin();
    display.setPoll(pollButtons);
    backlight.begin();

    // This order is the navigation order
    screenManager.addScreen(&homeScreen, "Home");
    screenManager.addScreen(&statsScreen, "Stats");
    screenManager.addScreen(&playScreen, "Play");
    screenManager.addScreen(&albumScreen, "Album");
    screenManager.openModal(&splashScreen);
    backlight.setFull();
    nextTickAt = millis() + 1000;
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
    devConsole();
#endif
    checkGestures(now);

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
        startRepeat(now, Key::CONFIRM);
    }
    checkRepeat(now);

    tickPet(now);
    screenManager.update(now);
    saver.update(now);
}
