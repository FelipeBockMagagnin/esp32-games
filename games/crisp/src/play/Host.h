#ifndef HOST_H
#define HOST_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ButtonManager.h"
#include "Cart.h"
#include "../core/SoundManager.h"

// The board's side of crisp-game-lib (lib/cglp's machineDependent.h): the frame, the buttons,
// the tones and the pause menu. The library draws into an indexed frame at the game's own
// size, at most 200x100, which goes out scaled: 2x when that fits the 240-wide panel, else
// stretched to its width. The strip below names the game and its keys.
class Host
{
public:
    struct Pins
    {
        Button &up, &down, &left, &right, &ok;
    };

    Host(TFT_eSPI &tft, Pins pins, SoundManager &sound, void (*quit)(uint16_t index),
         void (*scored)(uint16_t index));

    // Sets up DMA; false only means the slower blocking pushes
    bool begin();
    void start(uint16_t index);
    bool running() const { return active; }
    // Runs the game's next 1/60 s, when it is due
    void loop();

    void initView(int w, int h);
    void clearView(uint8_t r, uint8_t g, uint8_t b);
    void drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b);
    void drawCharacter(const unsigned char grid[6][6][3], float x, float y);
    void playTone(float freq, float duration, float when);
    void stopTone();
    float audioTime() const;
    // Plays the note due now; the board's timer calls it every 2 ms
    void soundTick();

    static const int FRAME_BYTES = 200 * 100;

private:
    enum MenuRow : uint8_t
    {
        ROW_RESUME,
        ROW_RESTART,
        ROW_QUIT,
        ROW_LAUNCHER,
        ROW_COUNT
    };

    static const int BAND_ROWS = 16;
    static const int MAX_BANDS = 320 / BAND_ROWS;
    static const int PALETTE_SIZE = 64;
    static const int TONE_COUNT = 64;

    TFT_eSPI &tft;
    Pins pins;
    SoundManager &sound;
    void (*quit)(uint16_t index);
    void (*scored)(uint16_t index);

    bool active = false;
    uint16_t index = 0;
    const Cart *cart = nullptr;

    uint8_t *frame = nullptr;
    int viewW = 100, viewH = 100;
    uint16_t palette[PALETTE_SIZE];
    uint32_t paletteRgb[PALETTE_SIZE];
    uint8_t paletteCount = 0;
    uint32_t lastRgb = 0xFFFFFFFF;
    uint8_t lastIndex = 0;

    int16_t outW = 0, outH = 0, gameX = 0, gameY = 0, stripY = 0;
    uint8_t xMap[240];
    uint8_t yMap[320];
    bool dma = false;
    uint16_t *band[2] = {};
    uint8_t bandBuffer = 0;
    uint32_t bandHash[MAX_BANDS];
    bool repaintAll = true;

    struct Tone
    {
        float freq, duration, when;
    };
    Tone tones[TONE_COUNT];
    uint8_t toneNext = 0;
    float noteUntil = 0;
    uint16_t noteHz = 0;
    unsigned long audioStartUs = 0;

    uint64_t nextFrameUs = 0;
    uint8_t lateFrames = 0;
    bool waitRelease = false;
    bool launcherShown = false;
    float pointerX = 50, pointerY = 50;
    uint8_t pointerHeld = 0;
    int shownHi = -1;

    unsigned long statFrames = 0, statShown = 0, statSince = 0;

    uint8_t colorIndex(uint8_t r, uint8_t g, uint8_t b);
    void readPins();
    Button &menuButton();
    void drawPointer();
    void present();
    void drawStrip();
    void drawHi();
    void drawMenu(uint8_t row);
    void pause();
};

#endif // HOST_H
