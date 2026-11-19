// The buzzer is silent on a PC
#ifndef VOLUME_H
#define VOLUME_H

#include <cstdint>

enum class Volume : uint8_t
{
    MUTE,
    SOFT,
    MEDIUM,
    LOUD,
    COUNT
};

static const Volume VOLUME_DEFAULT = Volume::MEDIUM;

inline const char *volumeName(Volume) { return "Medio"; }
inline uint16_t volumeDuty(Volume) { return 0; }
inline Volume loadVolume() { return VOLUME_DEFAULT; }
inline void saveVolume(Volume) {}
inline void setVolume(Volume) {}
inline void playTone(uint8_t, unsigned int, unsigned long = 0) {}
inline void stopTone(uint8_t) {}

#endif // VOLUME_H
