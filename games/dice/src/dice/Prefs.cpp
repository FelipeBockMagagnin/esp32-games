#include "Prefs.h"
#include <string.h>
#include "Raffle.h"
#include "Spinner.h"

void defaultPrefs(Prefs &prefs)
{
    memset(&prefs, 0, sizeof(prefs));
    prefs.version = PREFS_VERSION;
    defaultSpec(prefs.roll);
    prefs.drawFrom = 1;
    prefs.drawTo = 100;
    prefs.drawRepeats = 0;
    prefs.slices = DEFAULT_SLICES;
}

bool writePrefs(SlotStore &store, const Prefs &prefs)
{
    Prefs blob;
    memset(&blob, 0, sizeof(blob));
    blob.version = PREFS_VERSION;
    blob.roll = prefs.roll;
    blob.drawFrom = prefs.drawFrom;
    blob.drawTo = prefs.drawTo;
    blob.drawRepeats = prefs.drawRepeats;
    blob.slices = prefs.slices;
    return store.save(&blob, sizeof(blob));
}

bool readPrefs(SlotStore &store, Prefs &prefs)
{
    Prefs blob;
    if (!store.load(&blob, sizeof(blob)) || blob.version != PREFS_VERSION)
    {
        return false;
    }
    clampSpec(blob.roll);
    if (blob.drawFrom < RAFFLE_MIN || blob.drawFrom > RAFFLE_MAX)
    {
        blob.drawFrom = 1;
    }
    if (blob.drawTo < blob.drawFrom || blob.drawTo > RAFFLE_MAX)
    {
        blob.drawTo = blob.drawFrom;
    }
    blob.drawRepeats = blob.drawRepeats ? 1 : 0;
    if (blob.slices < MIN_SLICES || blob.slices > MAX_SLICES)
    {
        blob.slices = DEFAULT_SLICES;
    }
    prefs = blob;
    return true;
}
