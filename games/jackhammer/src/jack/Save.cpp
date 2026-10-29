#include "Save.h"
#include <string.h>

static uint32_t layoutFingerprint()
{
    return ((uint32_t)sizeof(SaveBlob) << 16) ^ ((uint32_t)STYLE_COUNT << 8) ^ SONG_COUNT;
}

bool writeSave(SlotStore &store, const Profile &profile)
{
    SaveBlob blob;
    // Zeroed first, so the padding is stable for the checksum
    memset(&blob, 0, sizeof(blob));
    blob.version = SAVE_VERSION;
    blob.layout = layoutFingerprint();
    blob.profile = profile;
    return store.save(&blob, sizeof(blob));
}

bool readSave(SlotStore &store, Profile &profile)
{
    SaveBlob blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != SAVE_VERSION || blob.layout != layoutFingerprint())
    {
        return false;
    }
    profile = blob.profile;
    sanitize(profile);
    return true;
}
