#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <Arduino.h>
#include "GameState.h"
#include "Inventory.h"
#include "CombatState.h"
#include "Achievements.h"
#include "Unlocks.h"
#include "Ascension.h"

// The whole save is one raw blob in NVS (Preferences). That only works because every
// Snapshot holds primitives and indices into the constexpr config tables, never a
// String or a pointer.
//
// Adding a field to any Snapshot changes the layout, so bump SAVE_VERSION. Adding a row
// to a config table changes it too, but `layout` already catches that: a save whose
// version or layout does not match is ignored and the game starts fresh.
static const uint16_t SAVE_VERSION = 7;

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the snapshots were built against
    GameState::Snapshot game;
    Inventory::Snapshot inventory;
    CombatState::Snapshot combat;
    Achievements::Snapshot achievements;
    Unlocks::Snapshot unlocks;
    Ascension::Snapshot ascension;
};

// Fills a blob from the live state. Zeroes it first, so padding bytes are stable and the
// checksum (and the unchanged-save check) only see real data.
void captureSave(SaveBlob &blob, const GameState &game, const Inventory &inventory,
                 const CombatState &combat, const Achievements &achievements, const Unlocks &unlocks,
                 const Ascension &ascension);

// Turns a captured save into the start of the next run: the gold, buildings, upgrades,
// mining and zone progress go back to a new game's, while the items, awards, opened
// screens and the ascension stay. Loading a zeroed snapshot is how a new game starts.
void clearRun(SaveBlob &blob);

// Restores the live state, in dependency order: combat re-enters its zone, which reads
// the player's stats from the inventory and the upgrades in the game state.
void applySave(const SaveBlob &blob, GameState &game, Inventory &inventory, CombatState &combat,
               Achievements &achievements, Unlocks &unlocks, Ascension &ascension);

// Both return false on any failure; loadGame also rejects a save that is the wrong size,
// fails its checksum, or has another version or layout, leaving `blob` unusable.
bool saveGame(const SaveBlob &blob);
bool loadGame(SaveBlob &blob);
void eraseSave();

// Idle gold alone is written once a minute, which keeps NVS wear to years of play;
// anything the player would hate to lose (a purchase, a drop, an unlock) brings the next
// save forward to a few seconds away.
class AutoSaver
{
public:
    AutoSaver(const GameState &game, const Inventory &inventory, const CombatState &combat,
              const Achievements &achievements, const Unlocks &unlocks, const Ascension &ascension);

    // Called once the state is loaded, with the blob it came from (nullptr for a new game)
    void begin(unsigned long now, const SaveBlob *loaded);
    void update(unsigned long now);

private:
    uint32_t milestoneSignature() const;

    const GameState &game;
    const Inventory &inventory;
    const CombatState &combat;
    const Achievements &achievements;
    const Unlocks &unlocks;
    const Ascension &ascension;

    SaveBlob buffer;
    SaveBlob lastSaved;
    unsigned long nextSaveAt;
    uint32_t savedMilestones;
};

#endif // SAVE_GAME_H
