// Peanut-GB's implementation, compiled once, as C (as it is written), and at -O2 while the rest
// of the firmware builds at -Os: the CPU loop is where every frame's time goes
#pragma GCC optimize("O2")

#include <stdint.h>

// GameBoy.cpp routes these to the running game's Apu
uint8_t audio_read(uint16_t addr);
void audio_write(uint16_t addr, uint8_t val);

#define ENABLE_SOUND 1
#include "../gb/peanut_gb.h"
