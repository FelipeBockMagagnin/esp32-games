#include "Records.h"
#include <stdio.h>
#include <string.h>

void defaultRecords(Records &records)
{
    memset(&records, 0, sizeof(records));
    records.version = RECORDS_VERSION;
}

bool writeRecords(SlotStore &store, const Records &records)
{
    Records blob = records;
    blob.version = RECORDS_VERSION;
    blob.pad = 0;
    blob.pad2 = 0;
    return store.save(&blob, sizeof(blob));
}

bool readRecords(SlotStore &store, Records &records)
{
    Records blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != RECORDS_VERSION)
    {
        return false;
    }
    if (blob.mode >= MODE_COUNT)
    {
        blob.mode = 0;
    }
    records = blob;
    return true;
}

bool updateBest(Records &records, Mode mode, uint32_t result)
{
    if (result == 0)
    {
        return false;
    }
    uint32_t &best = records.best[(uint8_t)mode];
    bool better = mode == Mode::SPRINT ? best == 0 || result < best : result > best;
    if (better)
    {
        best = result;
    }
    return better;
}

void formatTime(uint32_t tenths, char *out, uint8_t size)
{
    if (tenths == 0)
    {
        snprintf(out, size, "-:--.-");
        return;
    }
    snprintf(out, size, "%u:%02u.%u", (unsigned)(tenths / 600), (unsigned)((tenths / 10) % 60), (unsigned)(tenths % 10));
}
