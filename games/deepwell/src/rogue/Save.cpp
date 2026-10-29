#include "Save.h"
#include <string.h>

static uint32_t layoutFingerprint()
{
    return ((uint32_t)sizeof(SaveBlob) << 16) ^ ((uint32_t)ENEMY_COUNT << 8) ^ ITEM_COUNT;
}

void captureSave(SaveBlob &blob, const Run &run, const Records &records)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    if (!run.isDead())
    {
        run.save(blob.run);
    }
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
    records.runs++;
    if (run.getFloor() > records.bestFloor)
    {
        records.bestFloor = run.getFloor();
    }
    if (run.getScore() > records.bestScore)
    {
        records.bestScore = run.getScore();
        return true;
    }
    return false;
}
