#include "Save.h"
#include <string.h>
#include "Playlist.h"

void defaultSave(SaveData &save)
{
    memset(&save, 0, sizeof(save));
    save.version = SAVE_VERSION;
}

bool writeSave(SlotStore &store, const SaveData &save)
{
    SaveData blob = save;
    blob.version = SAVE_VERSION;
    memset(blob.pad, 0, sizeof(blob.pad));
    return store.save(&blob, sizeof(blob));
}

bool readSave(SlotStore &store, SaveData &save)
{
    SaveData blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != SAVE_VERSION)
    {
        return false;
    }
    if (blob.track >= TRACK_COUNT)
    {
        blob.track = 0;
    }
    if (blob.mode >= (uint8_t)Mode::COUNT)
    {
        blob.mode = 0;
    }
    save = blob;
    return true;
}
