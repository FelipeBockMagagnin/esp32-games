#include "Prefs.h"
#include <string.h>
#include "Palettes.h"

void defaultPrefs(Prefs &prefs)
{
    memset(&prefs, 0, sizeof(prefs));
    prefs.version = PREFS_VERSION;
}

bool writePrefs(SlotStore &store, const Prefs &prefs)
{
    Prefs blob = prefs;
    blob.version = PREFS_VERSION;
    return store.save(&blob, sizeof(blob));
}

bool readPrefs(SlotStore &store, Prefs &prefs)
{
    Prefs blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != PREFS_VERSION)
    {
        return false;
    }
    if (blob.mode >= (uint8_t)Mode::COUNT)
    {
        blob.mode = 0;
    }
    if (blob.palette >= PALETTE_COUNT)
    {
        blob.palette = 0;
    }
    prefs = blob;
    return true;
}
