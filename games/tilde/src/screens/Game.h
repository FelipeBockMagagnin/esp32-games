#ifndef SCREENS_GAME_H
#define SCREENS_GAME_H

#include <stdio.h>
#include "../pet/Pet.h"
#include "../pet/Album.h"
#include "../pet/Save.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// What every screen reaches: one reference instead of five in each constructor
struct Game
{
    Pet &pet;
    Album &album;
    SoundManager &sound;
    ScreenManager &screens;
    AutoSaver &saver;
};

// "12m", "5h 03m", "2d 04h"
inline void formatDuration(uint32_t secs, char *out, size_t len)
{
    uint32_t mins = secs / 60, hours = mins / 60, days = hours / 24;
    if (hours == 0)
    {
        snprintf(out, len, "%lum", (unsigned long)mins);
    }
    else if (days == 0)
    {
        snprintf(out, len, "%luh %02lum", (unsigned long)hours, (unsigned long)(mins % 60));
    }
    else
    {
        snprintf(out, len, "%lud %02luh", (unsigned long)days, (unsigned long)(hours % 24));
    }
}

#endif // SCREENS_GAME_H
