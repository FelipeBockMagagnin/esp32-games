#ifndef FLUID_PREFS_H
#define FLUID_PREFS_H

#include "SlotStore.h"

// The last mode and palette. A raw blob of primitives: adding a field means bumping
// PREFS_VERSION, and a mismatched record is dropped.
static const uint16_t PREFS_VERSION = 1;

enum class Mode : uint8_t
{
    INK,
    WATER,
    COUNT
};

struct Prefs
{
    uint16_t version;
    uint8_t mode;
    uint8_t palette;
};

void defaultPrefs(Prefs &prefs);
bool writePrefs(SlotStore &store, const Prefs &prefs);
// False, leaving `prefs` alone, when there is no record or it is from another version
bool readPrefs(SlotStore &store, Prefs &prefs);

#endif // FLUID_PREFS_H
