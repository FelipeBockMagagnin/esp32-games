#include "Save.h"
#include <string.h>

static uint32_t layoutFingerprint()
{
    return ((uint32_t)sizeof(SaveBlob) << 16) ^ ((uint32_t)FORM_COUNT << 8) ^ (uint32_t)NEED_COUNT;
}

void captureSave(SaveBlob &blob, const Pet &pet)
{
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    pet.save(blob.pet);
}

bool writeSave(SlotStore &store, const SaveBlob &blob)
{
    return store.save(&blob, sizeof(blob));
}

bool readSave(SlotStore &store, SaveBlob &blob)
{
    return store.load(&blob, sizeof(blob)) && blob.version == SAVE_VERSION && blob.layout == layoutFingerprint();
}

void AutoSaver::begin(unsigned long now)
{
    captureSave(lastSaved, pet);
    nextSaveAt = now + INTERVAL_MS;
}

void AutoSaver::soon(unsigned long now)
{
    if ((long)(nextSaveAt - (now + SOON_MS)) > 0)
    {
        nextSaveAt = now + SOON_MS;
    }
}

void AutoSaver::saveNow()
{
    captureSave(buffer, pet);
    if (memcmp(&buffer, &lastSaved, sizeof(buffer)) != 0 && writeSave(store, buffer))
    {
        lastSaved = buffer;
    }
}

void AutoSaver::update(unsigned long now)
{
    if ((long)(now - nextSaveAt) < 0)
    {
        return;
    }
    nextSaveAt = now + INTERVAL_MS;
    saveNow();
}
