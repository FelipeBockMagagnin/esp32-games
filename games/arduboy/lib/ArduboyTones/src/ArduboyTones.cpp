/**
 * @file ArduboyTones.cpp
 * \brief
 * ArduboyTones (BSD-3-Clause, LICENSE.txt) on the host's first channel. Upstream times the notes
 * with timer 3's interrupt; here the host's tick walks the sequence, so it never races the game.
 */

#include "ArduboyTones.h"
#include "ArduboyHost.h"

static bool (*outputEnabled)() = nullptr;
static const uint16_t *tonesStart = nullptr;
static const uint16_t *tonesIndex = nullptr;
static uint16_t inlineTones[MAX_TONES * 2 + 1];
static bool tonesPlaying = false;
static bool toneForever = false;
static unsigned long toneEndsUs = 0;

static void tick()
{
  if (tonesPlaying && !toneForever && (long)(micros() - toneEndsUs) >= 0) {
    ArduboyTones::nextTone();
  }
}

ArduboyTones::ArduboyTones(bool (*outEn)())
{
  outputEnabled = outEn;
  abHostTick = tick;
}

void ArduboyTones::tone(uint16_t freq, uint16_t dur)
{
  inlineTones[0] = freq;
  inlineTones[1] = dur;
  inlineTones[2] = TONES_END;
  tonesInRAM(inlineTones);
}

void ArduboyTones::tone(uint16_t freq1, uint16_t dur1, uint16_t freq2, uint16_t dur2)
{
  inlineTones[0] = freq1;
  inlineTones[1] = dur1;
  inlineTones[2] = freq2;
  inlineTones[3] = dur2;
  inlineTones[4] = TONES_END;
  tonesInRAM(inlineTones);
}

void ArduboyTones::tone(uint16_t freq1, uint16_t dur1, uint16_t freq2, uint16_t dur2, uint16_t freq3,
                        uint16_t dur3)
{
  inlineTones[0] = freq1;
  inlineTones[1] = dur1;
  inlineTones[2] = freq2;
  inlineTones[3] = dur2;
  inlineTones[4] = freq3;
  inlineTones[5] = dur3;
  inlineTones[6] = TONES_END;
  tonesInRAM(inlineTones);
}

void ArduboyTones::tones(const uint16_t *tones)
{
  tonesStart = tonesIndex = tones;
  tonesPlaying = true;
  nextTone();
}

void ArduboyTones::tonesInRAM(uint16_t *tonesRAM)
{
  tones(tonesRAM);
}

void ArduboyTones::noTone()
{
  tonesPlaying = false;
  abHostTone(0, 0);
}

void ArduboyTones::volumeMode(uint8_t) {}

bool ArduboyTones::playing()
{
  return tonesPlaying;
}

uint16_t ArduboyTones::getNext()
{
  return *tonesIndex++;
}

void ArduboyTones::nextTone()
{
  uint16_t freq = getNext();
  if (freq == TONES_REPEAT) {
    tonesIndex = tonesStart;
    freq = getNext();
  }
  if (freq == TONES_END) {
    noTone();
    return;
  }
  uint16_t dur = getNext();
  freq &= ~TONE_HIGH_VOLUME;
  bool on = outputEnabled == nullptr || outputEnabled();
  abHostTone(0, on ? freq : 0);
  toneForever = dur == 0;
  toneEndsUs = micros() + (unsigned long)dur * 1000000UL / 1024;
}
