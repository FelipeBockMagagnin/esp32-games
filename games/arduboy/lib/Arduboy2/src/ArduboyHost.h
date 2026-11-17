#ifndef ARDUBOY_HOST_H
#define ARDUBOY_HOST_H

#include <stdint.h>

// What the Arduboy2 port asks of the firmware (src/play/Host.cpp). The game owns the loop, so
// these calls are also where the firmware gets a turn: the pause menu, saving, the sound.

enum AbDisplayFlag : uint8_t
{
  AB_INVERT = 1,
  AB_ALL_ON = 2,
  AB_OFF = 4,
  AB_FLIP_V = 8,
  AB_FLIP_H = 16
};

// A whole 128x64 frame in the OLED's layout: 8 pages of 128 bytes, bit 0 the top row of a page
void abHostPaint(const uint8_t *image, uint8_t flags);
// Arduboy's button bits (A_BUTTON...) as the five buttons make them
uint8_t abHostButtons();
// The game is waiting for its next frame: about a millisecond
void abHostIdle();
// Channel 0 or 1, the Arduboy's two speaker pins; 0 Hz is silence
void abHostTone(uint8_t channel, uint16_t hz);
// Set by a sound library that times its notes (ArduboyTones); the host calls it whenever the
// game hands it a turn, so a note lasts to within a millisecond while the game waits for a frame
extern void (*abHostTick)();

#endif
