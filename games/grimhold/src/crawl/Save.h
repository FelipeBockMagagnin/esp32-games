#ifndef CRAWL_SAVE_H
#define CRAWL_SAVE_H

#include "Run.h"
#include "SlotStore.h"

// One raw blob, which only works because the snapshot holds primitives and indices into the
// config tables, never a String or a pointer. Adding a field means bumping SAVE_VERSION; adding a
// table row is caught by `layout`. A mismatched save is discarded.
static const uint16_t SAVE_VERSION = 1;

struct Records
{
    uint16_t runs;
    uint16_t bestKills;
    uint8_t bestFloor;
    uint8_t tutorialDone;
    uint8_t pad[2];
};

struct SaveBlob
{
    uint16_t version;
    uint32_t layout; // Fingerprint of the table sizes the snapshot was built against
    Run::Snapshot run; // floor 0: no run to continue
    Records records;
};

// Zeroes the blob first, so padding bytes are stable for the checksum. A dead run is saved as no
// run, so Continue disappears.
void captureSave(SaveBlob &blob, const Run &run, const Records &records);
bool writeSave(SlotStore &store, const SaveBlob &blob);
// False when there is no save or it is from another version or layout
bool readSave(SlotStore &store, SaveBlob &blob);

// Counts a finished run into the records; true when it went deeper than any before
bool recordRun(Records &records, const Run &run);

#endif // CRAWL_SAVE_H
