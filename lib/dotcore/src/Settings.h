#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// How long without a press before the backlight dims; 0 never does
static constexpr unsigned long DIM_AFTER_MS[] = {60000, 300000, 0};
static constexpr uint8_t DIM_OPTION_COUNT = sizeof(DIM_AFTER_MS) / sizeof(DIM_AFTER_MS[0]);
static const uint8_t DIM_DEFAULT = 1;

// Player preferences, under a key of their own so changing one never touches the save's
// version or layout. Erasing the save clears them too. New fields go at the end: an older,
// shorter record still loads, and the fields it lacks read as 0.
struct Settings
{
    uint8_t sound;    // SoundMode
    uint8_t dimAfter; // Index into DIM_AFTER_MS
    uint8_t theme;    // Index into THEMES (ui/Themes.h)
};

bool loadSettings(const char *nameSpace, Settings &out);
void saveSettings(const char *nameSpace, const Settings &in);

#endif // SETTINGS_H
