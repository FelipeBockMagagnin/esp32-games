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
    return store.save(&blob, sizeof(blob));
}

bool readRecords(SlotStore &store, Records &records)
{
    Records blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != RECORDS_VERSION)
    {
        return false;
    }
    if (blob.level >= LEVEL_COUNT)
    {
        blob.level = 0;
    }
    records = blob;
    return true;
}

void formatTime(uint16_t tenths, char *out, uint8_t size)
{
    if (tenths == 0)
    {
        snprintf(out, size, "-:--.-");
        return;
    }
    if (tenths > TIME_CAP)
    {
        tenths = TIME_CAP;
    }
    snprintf(out, size, "%u:%02u.%u", tenths / 600, (tenths / 10) % 60, tenths % 10);
}
