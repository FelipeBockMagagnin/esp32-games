#include "Save.h"
#include <string.h>

static uint32_t layoutFingerprint()
{
    return ((uint32_t)sizeof(SaveBlob) << 16) ^ ((uint32_t)SLOT_COUNT << 8) ^ ((uint32_t)TIER_COUNT << 4) ^
           ENEMY_KIND_COUNT;
}

void captureSave(SaveBlob &blob, const Run &run, const Records &records)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    run.save(blob.run);
    blob.records = records;
}

bool writeSave(SlotStore &store, const SaveBlob &blob)
{
    return store.save(&blob, sizeof(blob));
}

bool readSave(SlotStore &store, SaveBlob &blob)
{
    return store.load(&blob, sizeof(blob)) && blob.version == SAVE_VERSION && blob.layout == layoutFingerprint();
}

bool recordRun(Records &records, const Run &run)
{
    if (records.runs < UINT16_MAX)
    {
        records.runs++;
    }
    if (run.getKills() > records.bestKills)
    {
        records.bestKills = run.getKills();
    }
    if (run.getFloor() > records.bestFloor)
    {
        records.bestFloor = run.getFloor();
        return true;
    }
    return false;
}
