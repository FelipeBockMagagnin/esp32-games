#ifndef PET_SAVE_H
#define PET_SAVE_H

#include "Pet.h"
#include "SlotStore.h"

// The save is one raw blob, which only works because the snapshot holds primitives and indices
// into the config tables. Adding a field means bumping SAVE_VERSION; adding a form is caught by
// `layout`. A mismatched save is discarded.
static const uint16_t SAVE_VERSION = 1;

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the snapshot was built against
    Pet::Snapshot pet;
};

static_assert(sizeof(SaveBlob) <= SlotStore::MAX_SIZE, "the save must fit a slot");

// Zeroes the blob first, so padding bytes are stable for the checksum and the unchanged-save check
void captureSave(SaveBlob &blob, const Pet &pet);
bool writeSave(SlotStore &store, const SaveBlob &blob);
// False when there is no save or it is from another version or layout
bool readSave(SlotStore &store, SaveBlob &blob);

// Idle time is written once a minute, which keeps NVS wear to years of play; a care action or a
// pet event brings the next save forward to a few seconds away. The clock changes every second,
// so unlike a game with idle stretches every one of these writes is a real one.
class AutoSaver
{
public:
    AutoSaver(SlotStore &store, const Pet &pet) : store(store), pet(pet) {}

    void begin(unsigned long now);
    void update(unsigned long now);
    void soon(unsigned long now);
    void saveNow();

private:
    static const unsigned long INTERVAL_MS = 60000;
    static const unsigned long SOON_MS = 5000;

    SlotStore &store;
    const Pet &pet;
    SaveBlob buffer;
    SaveBlob lastSaved;
    unsigned long nextSaveAt = 0;
};

#endif // PET_SAVE_H
