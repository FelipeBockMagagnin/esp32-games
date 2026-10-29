#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <Arduino.h>
#include "World.h"
#include "Colony.h"

// The whole save is one raw blob in NVS (Preferences). That only works because every
// Snapshot holds primitives and indices into the constexpr config tables, never a
// String or a pointer.
//
// Adding a field to any Snapshot changes the layout, so bump SAVE_VERSION. Adding a row
// to a config table changes it too, but `layout` already catches that: a save whose
// version or layout does not match is ignored and the game starts fresh.
static const uint16_t SAVE_VERSION = 1;

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the snapshots were built against
    World::Snapshot world;
    Colony::Snapshot colony;
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

// Fills a blob from the live state. Zeroes it first, so padding bytes are stable and the
// checksum (and the unchanged-save check) only see real data.
void captureSave(SaveBlob &blob, const World &world, const Colony &colony);
// The world first: the critters stand on its tiles
void applySave(const SaveBlob &blob, World &world, Colony &colony);

// Both return false on any failure; loadGame also rejects a save that is the wrong size,
// fails its checksum, or has another version or layout, leaving `blob` unusable.
bool saveGame(const SaveBlob &blob);
bool loadGame(SaveBlob &blob);
void eraseSave();

// The ticking world alone is written once a minute, a flash-wear budget of years of
// play; anything the player did brings the next save forward to a few seconds away.
class AutoSaver
{
public:
    AutoSaver(const World &world, const Colony &colony);

    // Called once the state is loaded, with the blob it came from (nullptr for a new game)
    void begin(unsigned long now, const SaveBlob *loaded);
    void update(unsigned long now);

private:
    uint32_t milestoneSignature() const;

    const World &world;
    const Colony &colony;

    SaveBlob buffer;
    SaveBlob lastSaved;
    unsigned long nextSaveAt;
    uint32_t savedMilestones;
};

#endif // SAVE_GAME_H
