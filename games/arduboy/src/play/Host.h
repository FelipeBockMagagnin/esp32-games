#ifndef HOST_H
#define HOST_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ButtonManager.h"
#include "Sketch.h"
#include "../core/SoundManager.h"

// The board's side of the Arduboy2 port (lib/Arduboy2/src/ArduboyHost.h). A game owns the loop,
// so every call it makes into the library is the host's turn: buttons, the pause menu, saving.
//
// The 128x64 screen is drawn at 2x, 256x128, centred at the top; the strip below names the game
// and the controls, or holds the pause menu. Five buttons stand in for six: OK is A (or B, for
// a game whose main action is B), Left+Right together the other one. Holding Up+Down opens the
// menu. Quitting restarts the board, since the game's globals cannot be put back as they were.
class Host
{
public:
    struct Pins
    {
        Button &up, &down, &left, &right, &ok;
    };

    Host(TFT_eSPI &tft, Pins pins, const char *nameSpace, SoundManager &sound, void (*quit)(uint16_t index),
         void (*restart)(uint16_t index));

    // Sets up DMA; false only means the slower blocking pushes
    bool begin();
    // Loads the game's EEPROM and draws the frame; the caller then runs the game's setup()
    void start(uint16_t index);
    bool running() const { return game != nullptr; }
    const Sketch &sketch() const { return *game; }

    void paint(const uint8_t *image, uint8_t flags);
    uint8_t buttons();
    void idle();
    void tone(uint8_t channel, uint16_t hz);

private:
    enum MenuRow : uint8_t
    {
        ROW_RESUME,
        ROW_SWAP,
        ROW_RESTART,
        ROW_QUIT,
        ROW_LAUNCHER,
        ROW_COUNT
    };

    static const int SCALE = 2;
    static const int OUT_W = 128 * SCALE;
    static const int OUT_H = 64 * SCALE;
    static const int BAND_ROWS = 8 * SCALE; // One OLED page
    static const int PAGES = 8;

    TFT_eSPI &tft;
    Pins pins;
    const char *nameSpace;
    SoundManager &sound;
    void (*quit)(uint16_t index);
    void (*restart)(uint16_t index);

    const Sketch *game = nullptr;
    uint16_t index = 0;
    int16_t gameX = 0, gameY = 0, stripY = 0;
    bool dma = false;
    uint16_t band[2][OUT_W * BAND_ROWS];
    uint8_t bandBuffer = 0;
    uint8_t shown[128 * PAGES];
    uint8_t shownFlags = 0xFF;
    uint16_t onColor = 0, offColor = 0;

    uint16_t channelHz[2] = {};
    bool okIsB = false;
    bool waitRelease = false;
    bool menuChord = false;
    bool otherChord = false;
    bool launcherShown = false;
    unsigned long chordSince = 0;
    unsigned long lastPollMs = 0;
    uint32_t savedWrites = 0;
    uint32_t seenWrites = 0;
    unsigned long writtenAt = 0;
    const char *status = nullptr;
    unsigned long statusUntil = 0;

    unsigned long statFrames = 0;
    unsigned long statSince = 0;

    void poll();
    void readPins();
    void pushPage(uint8_t page, const uint8_t *image, uint8_t flags);
    void saveEeprom(bool quietly);
    void showStatus(const char *text);
    void drawStrip();
    void drawControl(const char *action, const char *keys, int16_t x, int16_t y);
    void drawMenu(uint8_t row);
    void pause();
};

#endif // HOST_H
