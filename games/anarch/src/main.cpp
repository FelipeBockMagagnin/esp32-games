#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include "Backlight.h"
#include "ButtonManager.h"
#include "Launcher.h"
#include "Volume.h"

// The board is held on its side, turned a quarter clockwise from the other games, so the D-pad
// sits under the left thumb. Each button is named for the way it points held like that: the
// arrow the other games call Up now points right.
static const uint8_t ROTATION = (DISPLAY_ROTATION + 1) % 4;
const int RIGHT_BUTTON_PIN = 19;
const int CONFIRM_BUTTON_PIN = 27;
const int UP_BUTTON_PIN = 26;
const int DOWN_BUTTON_PIN = 23;
const int LEFT_BUTTON_PIN = 22;
const int BUZZER_PIN = 32;

static const unsigned long DEBOUNCE_MS = 15;
static const char *const NVS_NAMESPACE = "anarch";
static const char *const SAVE_KEY = "save";

bool launcherShown = false;

#include "AnarchConfig.h"

static const int SRC_W = SFG_SCREEN_RESOLUTION_X;
static const int SRC_H = SFG_SCREEN_RESOLUTION_Y;
static const int OUT_W = SRC_W * 2;
static const int BAND_SRC_ROWS = 8;
static const int BAND_OUT_ROWS = BAND_SRC_ROWS * 2;

TFT_eSPI tft = TFT_eSPI();
Backlight backlight(TFT_BL);
Preferences prefs;
Button rightButton = Button(RIGHT_BUTTON_PIN, DEBOUNCE_MS);
Button confirmButton = Button(CONFIRM_BUTTON_PIN, DEBOUNCE_MS);
Button upButton = Button(UP_BUTTON_PIN, DEBOUNCE_MS);
Button downButton = Button(DOWN_BUTTON_PIN, DEBOUNCE_MS);
Button leftButton = Button(LEFT_BUTTON_PIN, DEBOUNCE_MS);

uint8_t screen[SRC_W * SRC_H];
uint16_t palette[256];
uint16_t band[2][OUT_W * BAND_OUT_ROWS];
bool dma = false;
int8_t keys[SFG_KEY_COUNT];
bool turnChord = false;
bool walkChord = false;

#ifdef DEV_MODE
unsigned long statFrames = 0;
unsigned long statSince = 0;
#endif

// A sound effect is a pitch sliding from one frequency to another, walked one loop at a time so
// only a change of note reaches the buzzer: playTone() queues its notes, and a queue of
// gunshots would lag behind the game.
struct Sweep
{
    uint16_t fromHz, toHz, ms;
};

// Indexed as sounds.h: shot, door, explosion, click, plasma, monster
static const Sweep SWEEPS[] = {
    {900, 140, 110}, {180, 110, 260}, {150, 40, 340}, {1800, 1800, 25}, {420, 1500, 150}, {220, 360, 120},
};
const Sweep *sweep = nullptr;
unsigned long sweepStart = 0;
uint8_t sweepVolume = 0;
uint16_t note = 0;

void playNote(uint16_t hz)
{
    if (hz == note)
    {
        return;
    }
    note = hz;
    if (hz == 0)
    {
        stopTone(BUZZER_PIN);
    }
    else
    {
        playTone(BUZZER_PIN, hz);
    }
}

void updateSound(unsigned long now)
{
    if (sweep == nullptr)
    {
        return;
    }
    unsigned long t = now - sweepStart;
    if (t >= sweep->ms)
    {
        sweep = nullptr;
        playNote(0);
        return;
    }
    int32_t hz = sweep->fromHz + ((int32_t)sweep->toHz - sweep->fromHz) * (int32_t)t / sweep->ms;
    playNote((uint16_t)hz);
}

void SFG_playSound(uint8_t soundIndex, uint8_t volume)
{
    if (soundIndex >= sizeof(SWEEPS) / sizeof(SWEEPS[0]))
    {
        return;
    }
    // A quieter sound never cuts a louder one short
    if (sweep != nullptr && volume < sweepVolume)
    {
        return;
    }
    sweep = &SWEEPS[soundIndex];
    sweepStart = millis();
    sweepVolume = volume;
    note = 0;
    updateSound(sweepStart);
}

void SFG_setMusic(uint8_t value)
{
}

void SFG_processEvent(uint8_t event, uint8_t data)
{
}

void SFG_setPixel(uint16_t x, uint16_t y, uint8_t colorIndex)
{
    screen[y * SRC_W + x] = colorIndex;
}

uint32_t SFG_getTimeMs()
{
    return millis();
}

void SFG_sleepMs(uint16_t timeMs)
{
}

void SFG_getMouseOffset(int16_t *x, int16_t *y)
{
}

int8_t SFG_keyPressed(uint8_t key)
{
    return key < SFG_KEY_COUNT ? keys[key] : 0;
}

void SFG_save(uint8_t data[SFG_SAVE_SIZE])
{
    prefs.putBytes(SAVE_KEY, data, SFG_SAVE_SIZE);
}

uint8_t SFG_load(uint8_t data[SFG_SAVE_SIZE])
{
    if (prefs.getBytesLength(SAVE_KEY) == SFG_SAVE_SIZE)
    {
        prefs.getBytes(SAVE_KEY, data, SFG_SAVE_SIZE);
    }
    return 1;
}

// Five buttons stand in for the game's seven. A chord mutes its two buttons until both are let
// go, so pressing them a moment apart neither turns nor walks.
void readKeys()
{
    bool up = upButton.isDown();
    bool down = downButton.isDown();
    bool left = leftButton.isDown();
    bool right = rightButton.isDown();
    bool ok = confirmButton.isDown();

    if (left && right)
    {
        turnChord = true;
    }
    else if (!left && !right)
    {
        turnChord = false;
    }
    if (up && down)
    {
        walkChord = true;
    }
    else if (!up && !down)
    {
        walkChord = false;
    }

    memset(keys, 0, sizeof(keys));
    keys[SFG_KEY_NEXT_WEAPON] = left && right;
    keys[SFG_KEY_MENU] = up && down;
    keys[SFG_KEY_LEFT] = left && !turnChord;
    keys[SFG_KEY_RIGHT] = right && !turnChord;
    keys[SFG_KEY_UP] = up && !walkChord;
    keys[SFG_KEY_DOWN] = down && !walkChord;
    // C + Up jumps and C holds the fire. OK with Down stays a shot backing off, since C + Down
    // would open the menu.
    bool jump = ok && keys[SFG_KEY_UP];
    keys[SFG_KEY_C] = jump;
    keys[SFG_KEY_A] = ok && !jump;
}

// The panel takes RGB565 high byte first, and the band goes out as it sits in memory
static uint16_t panelOrder(uint16_t c)
{
    return (uint16_t)(c << 8 | c >> 8);
}

// 160x120 doubled to the full 320x240, one band drawn while the last one goes out by DMA
void pushFrame()
{
    uint8_t buffer = 0;
    tft.startWrite();
    for (int top = 0; top < SRC_H; top += BAND_SRC_ROWS)
    {
        uint16_t *out = band[buffer];
        const uint8_t *src = screen + top * SRC_W;
        for (int row = 0; row < BAND_SRC_ROWS; row++)
        {
            uint16_t *line = out + row * 2 * OUT_W;
            for (int x = 0; x < SRC_W; x++)
            {
                uint16_t c = palette[*src++];
                line[x * 2] = c;
                line[x * 2 + 1] = c;
            }
            memcpy(line + OUT_W, line, OUT_W * sizeof(uint16_t));
        }
        if (dma)
        {
            tft.pushImageDMA(0, top * 2, OUT_W, BAND_OUT_ROWS, out);
            buffer ^= 1;
        }
        else
        {
            tft.pushImage(0, top * 2, OUT_W, BAND_OUT_ROWS, out);
        }
    }
    if (dma)
    {
        tft.dmaWait();
    }
    tft.endWrite();
}

void setup()
{
    Serial.begin(115200);

    rightButton.setup();
    confirmButton.setup();
    upButton.setup();
    downButton.setup();
    leftButton.setup();
    delay(100);

    // Holding Confirm while the board powers on opens the game menu, if the site installed one
    if (digitalRead(CONFIRM_BUTTON_PIN) == LOW)
    {
        returnToLauncher();
    }

    prefs.begin(NVS_NAMESPACE, false);
    // Holding Up and Down while the board powers on wipes the save
    if (digitalRead(UP_BUTTON_PIN) == LOW && digitalRead(DOWN_BUTTON_PIN) == LOW)
    {
        prefs.clear();
        Serial.println("Save erased");
    }

    launcherShown = launcherInstalled();
    for (int i = 0; i < 256; i++)
    {
        palette[i] = panelOrder(paletteRGB565[i]);
    }

    tft.init();
    tft.setRotation(ROTATION);
    dma = tft.initDMA();
    tft.fillScreen(TFT_BLACK);
    backlight.begin();
    backlight.setFull();

    SFG_init();
#ifdef DEV_MODE
    statSince = millis();
#endif
}

void loop()
{
    rightButton.loop();
    confirmButton.loop();
    upButton.loop();
    downButton.loop();
    leftButton.loop();
    readKeys();

    uint32_t frame = SFG_game.frame;
    if (!SFG_mainLoopBody())
    {
        playNote(0);
        returnToLauncher();
    }
    updateSound(millis());
    if (SFG_game.frame == frame)
    {
        return;
    }
    pushFrame();

#ifdef DEV_MODE
    statFrames++;
    unsigned long now = millis();
    if (now - statSince >= 5000)
    {
        Serial.printf("fps %.1f\n", statFrames * 1000.0f / (now - statSince));
        statFrames = 0;
        statSince = now;
    }
#endif
}
