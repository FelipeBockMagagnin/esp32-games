#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include "SlotStore.h"

// The last track and the play mode. A raw blob: adding a field means bumping SAVE_VERSION, and a
// mismatched record is dropped.
static const uint16_t SAVE_VERSION = 1;

struct SaveData
{
    uint16_t version;
    uint16_t track;
    uint8_t mode;
    uint8_t pad[3];
};

void defaultSave(SaveData &save);
bool writeSave(SlotStore &store, const SaveData &save);
// False, leaving `save` alone, when there is none or it does not fit this build's tracks
bool readSave(SlotStore &store, SaveData &save);

#endif // SAVE_H
