#include "Settings.h"
#include <Preferences.h>

static const char *const KEY_SETTINGS = "settings";

bool loadSettings(const char *nameSpace, Settings &out)
{
    Preferences prefs;
    if (!prefs.begin(nameSpace, true))
    {
        return false;
    }
    size_t stored = prefs.getBytesLength(KEY_SETTINGS);
    bool ok = stored > 0 && stored <= sizeof(out);
    if (ok)
    {
        Settings read = {};
        ok = prefs.getBytes(KEY_SETTINGS, &read, stored) == stored;
        if (ok)
        {
            out = read;
        }
    }
    prefs.end();
    return ok;
}

void saveSettings(const char *nameSpace, const Settings &in)
{
    Preferences prefs;
    if (prefs.begin(nameSpace, false))
    {
        prefs.putBytes(KEY_SETTINGS, &in, sizeof(in));
        prefs.end();
    }
}
