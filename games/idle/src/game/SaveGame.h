#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <Arduino.h>
#include "GameState.h"
#include "Inventory.h"
#include "CombatState.h"

// Nothing is persisted yet, but every piece of state is already shaped for it: each
// Snapshot holds only primitives and indices into the constexpr config tables, never a
// String or a pointer, so the whole blob can go to flash as raw bytes.
//
// Adding a field to any Snapshot changes the layout, so bump SAVE_VERSION and have
// loadGame() reject or migrate anything older.
static const uint16_t SAVE_VERSION = 1;

struct SaveBlob
{
    uint16_t version;
    GameState::Snapshot game;
    Inventory::Snapshot inventory;
    CombatState::Snapshot combat;
};

// TODO: implement over Preferences (NVS). Both are no-ops for now, so a boot always
// starts fresh; the call sites are what this header exists to pin down.
inline bool saveGame(const SaveBlob &blob)
{
    (void)blob;
    return false;
}

inline bool loadGame(SaveBlob &blob)
{
    (void)blob;
    return false;
}

#endif // SAVE_GAME_H
