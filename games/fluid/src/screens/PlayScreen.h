#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../fluid/FieldRender.h"
#include "../fluid/InkSim.h"
#include "../fluid/Prefs.h"
#include "../fluid/WaterSim.h"

// The field, opened as a modal from the title in the mode Prefs names. It owns the whole panel
// and repaints it every frame from update(), in bands that go out by DMA while the next one is
// drawn, so it has no elements. The simulations' memory is taken on entering and given back on
// leaving. Confirm freezes the field under a menu box.
class PlayScreen : public Screen
{
public:
    // Bits of setHeld()'s mask
    static const uint8_t HELD_UP = 1, HELD_DOWN = 2, HELD_LEFT = 4, HELD_RIGHT = 8;

    PlayScreen(TFT_eSPI &tft, Prefs &prefs, void (*save)(), ScreenManager &screenManager, SoundManager &sound);

    void setDma(bool on) { dma = on; }
    // The arrows held this frame: the ink's nozzle moves while they are
    void setHeld(uint8_t mask) { held = mask; }

    void onEnter(TFT_eSPI &tft) override;
    void onExit() override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;

    bool inMenu() const { return state == State::MENU; }
    const InkSim &inkSim() const { return ink; }
    const WaterSim &waterSim() const { return water; }

private:
    enum class State : uint8_t
    {
        RUNNING,
        MENU,
        NO_MEMORY
    };
    enum Option : uint8_t
    {
        RESUME,
        CLEAR,
        SWITCH,
        PALETTE,
        QUIT,
        OPTION_COUNT
    };
    static const int BAND_ROWS = 16;

    TFT_eSPI &tft;
    Prefs &prefs;
    void (*save)();
    ScreenManager &screenManager;
    SoundManager &sound;

    InkSim ink;
    WaterSim water;
    FieldRender render;
    uint16_t *bands[2] = {nullptr, nullptr};
    bool dma = false;

    State state = State::RUNNING;
    uint8_t choice = 0;
    uint8_t held = 0;
    unsigned long lastFrame = 0;
    uint32_t frames = 0;

    float nozzleX = 0, nozzleY = 0, nozzleVX = 0, nozzleVY = 0;
    float inkPhase = 0;
    int8_t fallX = 0, fallY = 1; // The way gravity pulls the water, one of the four arrows

    Mode mode() const { return (Mode)prefs.mode; }
    bool start();
    void stop();
    void reset();
    void frame();
    void moveNozzle();
    void tilt(int8_t dx, int8_t dy, uint8_t dir);
    void arrow(int8_t dx, int8_t dy, uint8_t dir);
    void paint();
    void drawNozzle(uint16_t *band, int top);
    void drawMenu();
    void drawNoMemory();
};

#endif // PLAY_SCREEN_H
