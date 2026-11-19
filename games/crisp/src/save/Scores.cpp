#include "Scores.h"
#include <Preferences.h>
#include "../play/Cart.h"
#include "../play/Crisp.h"

static const char *const KEY = "scores";

struct Entry
{
    uint32_t id;
    int32_t score;
};

static const uint16_t MAX_ENTRIES = 256;

void Scores::load(const char *nameSpace)
{
    Preferences prefs;
    if (!prefs.begin(nameSpace, true))
    {
        return;
    }
    static Entry entries[MAX_ENTRIES];
    size_t count = prefs.getBytes(KEY, entries, sizeof(entries)) / sizeof(Entry);
    prefs.end();
    for (uint16_t i = 0; i < CART_COUNT; i++)
    {
        for (size_t e = 0; e < count; e++)
        {
            if (entries[e].id == CARTS[i].id)
            {
                crispSetHiScore(i, entries[e].score);
            }
        }
    }
}

void Scores::store(const char *nameSpace)
{
    static Entry entries[MAX_ENTRIES];
    size_t count = 0;
    for (uint16_t i = 0; i < CART_COUNT && count < MAX_ENTRIES; i++)
    {
        if (crispHiScore(i) > 0)
        {
            entries[count++] = {CARTS[i].id, crispHiScore(i)};
        }
    }
    Preferences prefs;
    if (prefs.begin(nameSpace, false))
    {
        prefs.putBytes(KEY, entries, count * sizeof(Entry));
        prefs.end();
    }
}
