#ifndef MINES_RECORDS_H
#define MINES_RECORDS_H

#include "Field.h"
#include "SlotStore.h"

// The chosen level and, per level, the best time and the games won and played. A raw blob of
// primitives: adding a field means bumping RECORDS_VERSION, and a mismatched record is dropped.
static const uint16_t RECORDS_VERSION = 1;
// Tenths of a second; 0 is no time yet
static const uint16_t TIME_CAP = 59999;

struct Records
{
    uint16_t version;
    uint8_t level;
    uint8_t pad;
    uint16_t best[LEVEL_COUNT];
    uint16_t wins[LEVEL_COUNT];
    uint16_t plays[LEVEL_COUNT];
};

void defaultRecords(Records &records);
bool writeRecords(SlotStore &store, const Records &records);
// False, leaving `records` alone, when there is no record or it is from another version
bool readRecords(SlotStore &store, Records &records);

// "m:ss.t", or "-:--.-" for no time
void formatTime(uint16_t tenths, char *out, uint8_t size);

#endif // MINES_RECORDS_H
