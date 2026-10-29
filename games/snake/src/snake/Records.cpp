#include "Records.h"
#include <string.h>

void defaultRecords(Records &records)
{
    memset(&records, 0, sizeof(records));
    records.version = RECORDS_VERSION;
    records.level = 1;
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
        blob.level = 1;
    }
    records = blob;
    return true;
}
