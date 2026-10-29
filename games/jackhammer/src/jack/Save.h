#ifndef JACK_SAVE_H
#define JACK_SAVE_H

#include "Profile.h"
#include "../core/SlotStore.h"

// One raw blob, which only works because Profile holds primitives and indices into the
// config tables. Adding a field means bumping SAVE_VERSION; a new shop row changes `layout`.
// A mismatched save is discarded.
static const uint16_t SAVE_VERSION = 1;

struct SaveBlob
{
    uint16_t version;
    uint32_t layout;
    Profile profile;
};

bool writeSave(SlotStore &store, const Profile &profile);
// False, leaving the profile alone, when there is no save or it is from another version or layout
bool readSave(SlotStore &store, Profile &profile);

#endif // JACK_SAVE_H
