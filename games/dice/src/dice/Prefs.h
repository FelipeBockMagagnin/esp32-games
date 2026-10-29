#ifndef DICE_PREFS_H
#define DICE_PREFS_H

#include "Roll.h"
#include "SlotStore.h"

// What each tool was last set to, so the board wakes up on the same dice. A raw blob of
// primitives: adding a field means bumping PREFS_VERSION, and a mismatched record is dropped.
static const uint16_t PREFS_VERSION = 1;

struct Prefs
{
    uint16_t version;
    RollSpec roll;
    int16_t drawFrom;
    int16_t drawTo;
    uint8_t drawRepeats;
    uint8_t slices;
};

void defaultPrefs(Prefs &prefs);
// Zeroes the padding, so the checksum only sees the fields
bool writePrefs(SlotStore &store, const Prefs &prefs);
// False, leaving `prefs` alone, when there is no record or it is from another version;
// a record that loads is clamped back into range
bool readPrefs(SlotStore &store, Prefs &prefs);

#endif // DICE_PREFS_H
