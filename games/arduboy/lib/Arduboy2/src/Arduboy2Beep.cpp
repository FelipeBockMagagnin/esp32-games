/**
 * @file Arduboy2Beep.cpp
 * \brief
 * BeepPin1 and BeepPin2 on the host's two channels. The counts are the Arduboy's timer
 * compare values, turned back into hertz.
 */

#include <Arduino.h>
#include "Arduboy2Beep.h"
#include "Arduboy2Audio.h"
#include "ArduboyHost.h"

static void sound(uint8_t channel, uint16_t hz)
{
  abHostTone(channel, Arduboy2Audio::enabled() ? hz : 0);
}

uint8_t BeepPin1::duration = 0;

void BeepPin1::begin() {}

void BeepPin1::tone(uint16_t count)
{
  tone(count, 0);
}

void BeepPin1::tone(uint16_t count, uint8_t dur)
{
  duration = dur;
  sound(0, (uint16_t)(1000000UL / ((uint32_t)count + 1)));
}

void BeepPin1::timer()
{
  if (duration && (--duration == 0)) {
    sound(0, 0);
  }
}

void BeepPin1::noTone()
{
  duration = 0;
  sound(0, 0);
}

uint8_t BeepPin2::duration = 0;

void BeepPin2::begin() {}

void BeepPin2::tone(uint16_t count)
{
  tone(count, 0);
}

void BeepPin2::tone(uint16_t count, uint8_t dur)
{
  duration = dur;
  sound(1, (uint16_t)(62500UL / ((uint32_t)count + 1)));
}

void BeepPin2::timer()
{
  if (duration && (--duration == 0)) {
    sound(1, 0);
  }
}

void BeepPin2::noTone()
{
  duration = 0;
  sound(1, 0);
}
