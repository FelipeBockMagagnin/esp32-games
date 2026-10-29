#ifndef PET_CONFIG_H
#define PET_CONFIG_H

#include <stdint.h>

// Everything a pet's day is tuned with. Times are in seconds of powered time: the board has
// no clock across power-off, so a pet lives only while it is on. The numbers in these comments
// come from tools/sim.sh; rerun it after touching this file. Its current arc: hatched in
// minutes, a child after 3 h, an adult on day 2; an hourly owner who plays gets a Nimbus, one
// who feeds treats a Puddle, four visits a day a Pebble; the hourly owner can pass on an egg
// around day 8; four visits a day leave it sick about 12% of the time, and never worse.

enum Stage : uint8_t
{
    STAGE_EGG,
    STAGE_BABY,
    STAGE_CHILD,
    STAGE_ADULT,
    STAGE_COUNT
};

static const char *const STAGE_NAMES[STAGE_COUNT] = {"Egg", "Baby", "Child", "Adult"};

// FormId must stay in FORMS order, and so must FORM_RIG (art/Rig.cpp)
enum FormId : uint8_t
{
    FORM_EGG,
    FORM_BLIP,
    FORM_MOCHI,
    FORM_TUFT,
    FORM_PUDDLE,
    FORM_PEBBLE,
    FORM_NIMBUS,
    FORM_SPROUT,
    FORM_COUNT
};

struct FormDef
{
    const char *name; // 8 chars max
    Stage stage;
    const char *note; // Album line, 25 chars max
};

static constexpr FormDef FORMS[] = {
    {"Egg", STAGE_EGG, "Warm, and a little wobbly"},
    {"Blip", STAGE_BABY, "Every glyph starts as one"},
    {"Mochi", STAGE_CHILD, "Grew up on good meals"},
    {"Tuft", STAGE_CHILD, "Grew up playing"},
    {"Puddle", STAGE_ADULT, "A giggling jelly"},
    {"Pebble", STAGE_ADULT, "Shy, but always there"},
    {"Nimbus", STAGE_ADULT, "Floats and hums all day"},
    {"Sprout", STAGE_ADULT, "Curious about everything"},
};
static constexpr uint8_t FORM_COUNT_CHECK = sizeof(FORMS) / sizeof(FORMS[0]);
static_assert(FORM_COUNT_CHECK == FORM_COUNT, "a FORMS row per FormId");

enum Need : uint8_t
{
    NEED_FOOD,
    NEED_JOY,
    NEED_REST,
    NEED_COUNT
};

struct NeedDef
{
    const char *label;          // 4 chars
    const char *wants;          // "Mochi <wants>", for the call
    uint16_t awakeSecsPerPoint; // full to empty in 100 of these
    uint16_t asleepSecsPerPoint; // 0 holds still while asleep
};

static constexpr NeedDef NEEDS[NEED_COUNT] = {
    {"FOOD", "is hungry", 360, 720}, // 10 h awake, 20 asleep
    {"JOY", "wants to play", 288, 0}, // 8 h
    {"REST", "is sleepy", 144, 0},    // 4 h awake; sleep refills it (REST_REFILL_*)
};

static const uint8_t NEED_MAX = 100;
static const uint8_t NEED_LOW = 25;  // Sad below, and it calls
static const uint8_t NEED_OK = 50;   // Meh below; care given below it counts double for the bond
static const uint8_t NEED_HIGH = 75; // Happy with every need above

// Seconds of sleep per point of rest: about an hour in the dark, two with the lamp on
static const uint16_t REST_REFILL_DARK = 36;
static const uint16_t REST_REFILL_LIT = 72;
// The lamp off with this little rest left puts it to bed
static const uint8_t REST_BEDTIME = 50;
// With the lamp off it sleeps on once rested, through the owner's night, until the lamp goes
// on or it has slept this long
static const uint16_t DARK_SLEEP_MAX_SECS = 36000;

static const uint8_t MEAL_FOOD = 60;
static const uint8_t SNACK_FOOD = 15;
static const uint8_t SNACK_JOY = 10;
// A meal past this is refused: it is full
static const uint8_t FULL_FOOD = 90;
// Snacks this close together give it a tummy ache; one is forgiven every SNACK_COOL_SECS
static const uint8_t SNACK_LIMIT = 4;
static const uint16_t SNACK_COOL_SECS = 900;

static const uint8_t CUDDLE_JOY = 25;
static const uint8_t CUDDLE_REPEAT_JOY = 3; // within the cooldown
static const uint16_t CUDDLE_COOLDOWN_SECS = 300;

// A minigame, whatever the score: at least this much joy, and it tires
static const uint8_t PLAY_MIN_JOY = 30;
static const uint8_t PLAY_MAX_JOY = 60;
static const uint8_t PLAY_REST_COST = 6;
static const uint8_t PLAY_MIN_REST = 10;

// It sheds a little fluff now and then while awake, which Clean sweeps up
static const uint8_t MAX_FLUFF = 3;
static const uint16_t FLUFF_EVERY_SECS = 9000;
static const uint16_t FLUFF_JITTER_SECS = 2700;

// Two needs low this long while awake make it sick; Meds cures at once. Sick, joy drops twice
// as fast and the bond grows at half speed, and that is all: a pet never dies or leaves.
static const uint16_t SICK_AFTER_SECS = 7200;

static const uint16_t EGG_SECS = 180;
static const uint8_t WARM_SECS = 2; // each press on the egg (a held Confirm repeats it)
static const uint32_t BABY_SECS = 10800;
static const uint32_t CHILD_SECS = 86400;

// The bond grows with care and is what an adult's form is chosen by. It drops a little in an
// hour of neglect, never below half its peak.
static const uint16_t BOND_MAX = 1000;
static const uint8_t BOND_CARE = 6;  // care given while the need was below NEED_OK
static const uint8_t BOND_TOUCH = 1; // any other care
static const uint8_t BOND_HOURLY = 4;  // an hour with every need at NEED_OK or more
static const uint8_t BOND_NEGLECT = 2; // an hour with two needs low
static const uint16_t ADULT_HIGH_BOND = 85; // hourly owners reach ~90-115 by then, four visits a day ~60

// An adult this old and this close can leave an egg and go to the Album
static const uint32_t LEAVE_EGG_ADULT_SECS = 7 * 86400UL;
static const uint16_t LEAVE_EGG_BOND = 600;

static const uint8_t NAME_LEN = 8;
static const char *const DEFAULT_NAMES[] = {"Momo", "Bean", "Pip", "Nori", "Tofu", "Miso", "Dot", "Kiwi",
                                            "Puff", "Yuzu", "Sumi", "Taro"};
static constexpr uint8_t DEFAULT_NAME_COUNT = sizeof(DEFAULT_NAMES) / sizeof(DEFAULT_NAMES[0]);

// Where the fluff can land: columns of the room clear of the plant (art/Room)
static const uint8_t ROOM_SPOTS = 28;

#endif // PET_CONFIG_H
