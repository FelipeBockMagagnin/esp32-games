#ifndef ROGUE_SAVE_H
#define ROGUE_SAVE_H

#include "Run.h"
#include "SlotStore.h"

// The save is one raw blob, which only works because the snapshots hold primitives and
// indices into the config tables, never a String or a pointer. Adding a field means bumping
// SAVE_VERSION; adding a table row is caught by `layout`. A mismatched save is discarded.
static const uint16_t SAVE_VERSION = 1;

struct Records
{
    uint32_t bestScore;
    uint16_t runs;
    uint8_t bestFloor;
};

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the snapshots were built against
    Run::Snapshot run;
    Records records;
};

// Zeroes the blob first, so padding bytes are stable for the checksum. A dead run is
// saved as no run, so the next boot starts a new one.
void captureSave(SaveBlob &blob, const Run &run, const Records &records);
bool writeSave(SlotStore &store, const SaveBlob &blob);
// False when there is no save or it is from another version or layout
bool readSave(SlotStore &store, SaveBlob &blob);

// Counts a finished run into the records; true when it set a new best score
bool recordRun(Records &records, const Run &run);

#endif // ROGUE_SAVE_H
