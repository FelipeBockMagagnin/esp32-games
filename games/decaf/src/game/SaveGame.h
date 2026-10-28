#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <Arduino.h>
#include "Levels.h"

// The whole save is one raw blob in NVS (Preferences): primitives only, never a String or a
// pointer. Adding a field means bumping SAVE_VERSION; adding a floor to LEVELS changes `layout`.
// A save whose version or layout does not match is ignored and the records start fresh.
static const uint16_t SAVE_VERSION = 1;

struct FloorRecord
{
    uint32_t bestTicks; // Steps of play on the fastest clear; 0 until the floor is cleared
    uint16_t fewestDeaths;
    uint16_t clears;
};

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the records were built against
    FloorRecord floors[LEVEL_COUNT];
};

// Player preferences, under a key of their own so changing one never touches the save's
// version or layout. Erasing the save clears them too. New fields go at the end: an older,
// shorter record still loads, and the fields it lacks read as 0.
struct Settings
{
    uint8_t sound;    // SoundMode
    uint8_t dimAfter; // Index into DIM_AFTER_MS (SettingsScreen.h)
    uint8_t theme;    // Index into THEMES (ui/Themes.h)
};
bool loadSettings(Settings &out);
void saveSettings(const Settings &in);

// A blank save of this firmware's version and layout
void newSave(SaveBlob &blob);
// Adds a clear to the floor's record; true when it is a new best time
bool recordClear(SaveBlob &blob, uint8_t level, uint32_t ticks, uint16_t deaths);

// Both return false on any failure; loadGame also rejects a save that is the wrong size,
// fails its checksum, or has another version or layout, leaving `blob` unusable.
bool saveGame(const SaveBlob &blob);
bool loadGame(SaveBlob &blob);
void eraseSave();

#endif // SAVE_GAME_H
