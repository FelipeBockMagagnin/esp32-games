#ifndef STACK_RECORDS_H
#define STACK_RECORDS_H

#include "Game.h"
#include "SlotStore.h"

// The chosen mode and, per mode, the best run and the runs played. A raw blob of primitives:
// adding a field means bumping RECORDS_VERSION, and a mismatched record is dropped.
static const uint16_t RECORDS_VERSION = 1;

struct Records
{
    uint16_t version;
    uint8_t mode;
    uint8_t pad;
    uint32_t best[MODE_COUNT]; // A score, but Sprint's is tenths of a second, lower being better; 0 is none
    uint16_t plays[MODE_COUNT];
    uint16_t pad2;
};

void defaultRecords(Records &records);
bool writeRecords(SlotStore &store, const Records &records);
// False, leaving `records` alone, when there is no record or it is from another version
bool readRecords(SlotStore &store, Records &records);

// Stores `result` if it beats the mode's best, and says so
bool updateBest(Records &records, Mode mode, uint32_t result);

// "m:ss.t", or "-:--.-" for no time
void formatTime(uint32_t tenths, char *out, uint8_t size);

#endif // STACK_RECORDS_H
