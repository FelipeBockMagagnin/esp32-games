#ifndef VOLUME_H
#define VOLUME_H

#include <Arduino.h>

// One volume for every game, chosen in the launcher and kept under its namespace.
// The core's tone() always drives the buzzer at 50% duty, its loudest; playTone() is the same
// queue of notes on LEDC channel 0, with the duty set from the volume instead.
enum class Volume : uint8_t
{
    MUTE,
    SOFT,
    MEDIUM,
    LOUD,
    COUNT
};

static const Volume VOLUME_DEFAULT = Volume::MEDIUM;

const char *volumeName(Volume v);
// LEDC duty at 10 bits, the resolution ledcWriteTone() leaves the channel in
uint16_t volumeDuty(Volume v);

Volume loadVolume();
void saveVolume(Volume v);

// The first note loads the saved volume, unless setVolume() came first
void setVolume(Volume v);
// Queued like tone(): returns at once and the notes play one after another. `ms` 0 holds the
// note until stopTone().
void playTone(uint8_t pin, unsigned int hz, unsigned long ms = 0);
void stopTone(uint8_t pin);

#endif // VOLUME_H
