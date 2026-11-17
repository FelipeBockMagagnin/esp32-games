#ifndef GAME_BOY_H
#define GAME_BOY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "Apu.h"

#define PEANUT_GB_HEADER_ONLY
extern "C"
{
#include "../gb/peanut_gb.h"
}

// One running game: Peanut-GB reading the ROM straight from flash, the cartridge RAM on the
// heap, and the sound registers kept for the buzzer. Only one runs at a time.
class GameBoy
{
public:
    // Receives each of the 144 lines as Peanut-GB draws it, the shade in each pixel's low bits
    typedef void (*LineSink)(void *context, const uint8_t *pixels, uint8_t line);

    // False when the ROM is refused or its cartridge RAM does not fit in memory
    bool start(const uint8_t *rom, LineSink sink, void *context);
    void stop();
    bool running() const { return started; }

    // JOYPAD_* bits of the buttons held
    void setButtons(uint8_t held) { gb.direct.joypad = (uint8_t)~held; }
    // Draws every other frame only, for a panel slower than the emulation
    void setFrameSkip(bool on) { gb.direct.frame_skip = on; }
    void runFrame();
    // A CPU error stops the game; Peanut-GB cannot go on after one
    bool crashed() const { return error; }

    // The battery-backed RAM a save keeps: empty for a cartridge without a battery
    uint8_t *saveData() { return battery ? ram : nullptr; }
    size_t saveSize() const { return battery ? ramSize : 0; }
    // Written since clean(); a save waits for quietFrames() to grow, so it never lands mid-write
    bool dirty() const { return changed; }
    uint32_t quietFrames() const { return framesSinceWrite; }
    void clean() { changed = false; }

    Apu &sound() { return apu; }

private:
    struct gb_s gb;
    LineSink sink = nullptr;
    void *context = nullptr;
    const uint8_t *rom = nullptr;
    uint8_t *ram = nullptr;
    size_t ramSize = 0;
    bool battery = false;
    bool started = false;
    bool error = false;
    bool changed = false;
    uint32_t framesSinceWrite = 0;
    Apu apu;

    static uint8_t romRead(struct gb_s *gb, const uint_fast32_t addr);
    static uint8_t ramRead(struct gb_s *gb, const uint_fast32_t addr);
    static void ramWrite(struct gb_s *gb, const uint_fast32_t addr, const uint8_t value);
    static void onError(struct gb_s *gb, const enum gb_error_e error, const uint16_t addr);
    static void drawLine(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line);
};

#endif // GAME_BOY_H
