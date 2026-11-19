#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../core/SoundManager.h"
#include "../emu/GameBoy.h"
#include "../emu/Pack.h"
#include "../emu/Scale.h"

// The buttons as main.cpp reads them: levels for the game, presses for the pause menu
struct Pad
{
    bool up, down, left, right, ok;
    bool upPressed, downPressed, okPressed;
};

// A game running in full screen, outside the screens: it needs the buttons' levels every frame
// and the whole SPI bus. The game fills the top 240x216 at 1.5x; the strip below names it and
// shows the controls, or the pause menu.
//
// Five buttons stand in for eight: OK is A (or B, for a game whose main action is B),
// Left+Right together the other one, Up+Down tapped together Start. Holding Up+Down opens the
// menu. A chord stays a chord until both its buttons are up, so letting go of one first never
// sends its partner's direction.
class Player
{
public:
    Player(TFT_eSPI &tft, const Pack &pack, const char *nameSpace, SoundManager &sound, void (*quit)(),
           void (*toLauncher)());

    // Sets up DMA; false only means the slower blocking pushes
    bool begin();
    bool start(uint16_t index);
    bool active() const { return gb.running(); }
    void loop(const Pad &pad);

private:
    enum MenuRow : uint8_t
    {
        ROW_RESUME,
        ROW_SELECT,
        ROW_SWAP,
        ROW_SAVE,
        ROW_QUIT,
        ROW_LAUNCHER,
        ROW_COUNT
    };

    static const int BAND_ROWS = 24; // Output rows per DMA push: 16 Game Boy lines
    static const int STRIP_Y = Scale::OUT_H;

    TFT_eSPI &tft;
    const Pack &pack;
    const char *nameSpace;
    SoundManager &sound;
    void (*quit)();
    void (*toLauncher)();

    GameBoy gb;
    PackEntry game = {};
    bool dma = false;
    uint16_t shades[4] = {};
    uint16_t band[2][Scale::OUT_W * BAND_ROWS];
    uint8_t bandBuffer = 0;
    bool writing = false;

    bool okIsB = false;
    bool paused = false;
    uint8_t menuRow = 0;
    bool launcherShown = false;
    bool waitRelease = false;
    bool startChord = false;
    bool otherChord = false;
    unsigned long chordSince = 0;
    uint8_t selectFrames = 0;
    uint8_t startFrames = 0;
    unsigned long nextFrameUs = 0;
    unsigned long statusUntil = 0;
    const char *status = nullptr;

    unsigned long statFrames = 0;
    unsigned long statSince = 0;

    static void onLine(void *context, const uint8_t *pixels, uint8_t line);
    void line(const uint8_t *pixels, uint8_t line);
    void finishFrame();

    uint8_t joypad(const Pad &pad);
    void save(bool quietly);
    void showStatus(const char *text);
    void drawStrip();
    void drawControl(const char *action, const char *keys, int16_t x, int16_t y);
    void drawMenu();
    void openMenu();
    void menuInput(const Pad &pad);
    void end(bool launcher);
};

#endif // PLAYER_H
