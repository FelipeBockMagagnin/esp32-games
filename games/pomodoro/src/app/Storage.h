#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "FocusStats.h"

// Bump when Snapshot or Settings change layout: a mismatched record is ignored
static const uint16_t STATS_VERSION = 1;

// Index fields point into the option tables in SettingsScreen.h. New fields go at the end: an
// older, shorter record still loads, and the fields it lacks read as 0.
struct Settings
{
    uint8_t sound;    // SoundMode
    uint8_t dimAfter;
    uint8_t theme;    // Index into THEMES (ui/Themes.h)
    uint8_t focusMin;
    uint8_t breakMin;
    uint8_t longBreakMin;
    uint8_t autoStart;
};
bool loadSettings(Settings &out);
void saveSettings(const Settings &in);

// Two slots written in turn, each with a sequence number and a checksum, so a power cut
// during a write only loses that write
bool loadStats(FocusStats::Snapshot &out);
bool saveStats(const FocusStats::Snapshot &in);
void eraseStats();

#endif // STORAGE_H
