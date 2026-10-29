#ifndef SCREENS_LINES_H
#define SCREENS_LINES_H

#include "../pet/Pet.h"

// What the pet's speech bubble says: "%s" is the pet's name. Printable ASCII only (the font's),
// and 210 pixels at most in FONT_SMALL once an 8-letter name is in.
static const uint8_t LINES_PER_MOOD = 4;

static const char *const MOOD_LINES[(uint8_t)Mood::COUNT][LINES_PER_MOOD] = {
    // ASLEEP
    {"%s is fast asleep.", "%s dreams of berries.", "%s snores, very softly.", "Shh. %s is sleeping."},
    // SICK
    {"%s feels unwell. Meds?", "%s sniffles quietly.", "%s needs a little medicine.", "%s is a bit pale."},
    // SLEEPY
    {"%s yawns. Time for bed?", "%s's eyes keep closing.", "%s rubs its eyes.", "%s would nap in the dark."},
    // SAD
    {"%s looks a little sad.", "%s could use some care.", "%s sighs.", "%s looks at you hopefully."},
    // MEH
    {"%s is doing okay.", "%s sits and waits.", "%s looks around the room.", "%s hums, a bit flat."},
    // CONTENT
    {"%s watches the dust motes.", "%s hums a tiny song.", "%s is comfy.", "%s looks out the window."},
    // HAPPY
    {"%s wiggles happily!", "%s is having a great day.", "%s does a little dance.", "%s beams at you."},
};

static const char *const ACTION_LINES[(uint8_t)Action::COUNT] = {
    "%s munches its meal.",    // MEAL
    "%s loves a treat!",       // SNACK
    "%s snuggles up.",         // CUDDLE
    "All tidy. %s is pleased.", // CLEAN
    "",                        // LIGHT: by the lamp's state
    "%s feels much better!",   // MEDS
    "The egg feels warm.",     // WARM
};

static const char *const REFUSAL_LINES[] = {
    "",                        // NONE
    "Only warmth for an egg.", // EGG
    "",                        // NOT_EGG
    "Shh, %s is sleeping.",    // ASLEEP
    "%s is full.",             // FULL
    "Nothing to sweep.",       // NO_FLUFF
    "%s is not sick.",         // NOT_SICK
    "%s is too sleepy.",       // TIRED
};

#endif // SCREENS_LINES_H
