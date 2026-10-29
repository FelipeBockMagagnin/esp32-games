#ifndef SNAKE_RECORDS_H
#define SNAKE_RECORDS_H

#include "Snake.h"
#include "SlotStore.h"

// The chosen level and the best score on each. A raw blob of primitives: adding a field means
// bumping RECORDS_VERSION, and a mismatched record is dropped.
static const uint16_t RECORDS_VERSION = 1;

struct Records
{
    uint16_t version;
    uint8_t level;
    uint8_t pad;
    uint16_t best[LEVEL_COUNT];
};

void defaultRecords(Records &records);
bool writeRecords(SlotStore &store, const Records &records);
// False, leaving `records` alone, when there is no record or it is from another version
bool readRecords(SlotStore &store, Records &records);

#endif // SNAKE_RECORDS_H
