#ifndef PET_CONFIG_H
#define PET_CONFIG_H

#include <stdint.h>

enum FormId : uint8_t
{
    FORM_EGG,
    FORM_BABY,
    FORM_CHILD,
    FORM_TEEN_A,
    FORM_TEEN_B,
    FORM_ADULT_A,
    FORM_ADULT_B,
    FORM_ADULT_C,
    FORM_ADULT_D,
    FORM_ADULT_E,
    FORM_ADULT_F,
    FORM_SECRET,
    FORM_COUNT
};

enum class Stage : uint8_t
{
    EGG,
    BABY,
    CHILD,
    TEEN,
    ADULT
};

struct FormDef
{
    const char *name; // 8 chars at most: the matrix fits no more
    Stage stage;
    uint16_t hungerSeconds; // One heart, awake
    uint16_t happySeconds;
    uint16_t poopSeconds;
    uint8_t sleepHour; // sleepHour == wakeHour never sleeps
    uint8_t wakeHour;
    uint8_t baseWeight;
    uint8_t lifeDays; // Before care mistakes take their share; adults only
};

static constexpr FormDef FORMS[FORM_COUNT] = {
    {"OVO", Stage::EGG, 0, 0, 0, 0, 0, 5, 0},
    {"PINGO", Stage::BABY, 300, 360, 1500, 0, 0, 5, 0},
    {"TICO", Stage::CHILD, 2400, 2700, 5400, 20, 9, 10, 0},
    {"BICO", Stage::TEEN, 3000, 3300, 7200, 21, 9, 20, 0},
    {"TOCO", Stage::TEEN, 2700, 3000, 6300, 21, 9, 20, 0},
    {"ZUZU", Stage::ADULT, 3900, 3900, 9000, 22, 8, 30, 20},
    {"BABU", Stage::ADULT, 3600, 3600, 8400, 22, 8, 25, 18},
    {"GOMO", Stage::ADULT, 3300, 3300, 7800, 21, 9, 30, 15},
    {"NHOCA", Stage::ADULT, 3000, 3000, 7200, 23, 9, 30, 12},
    {"FIAPO", Stage::ADULT, 2700, 3000, 6600, 21, 9, 40, 10},
    {"TRECO", Stage::ADULT, 2400, 2700, 6000, 23, 10, 15, 8},
    {"REIZINHO", Stage::ADULT, 4500, 4500, 9600, 22, 8, 35, 28},
};

static const uint32_t DAY_SECONDS = 86400;
static const uint32_t EGG_SECONDS = 300;
static const uint32_t BABY_SECONDS = 3600;
static const uint32_t CHILD_SECONDS = 86400;
static const uint32_t TEEN_SECONDS = 172800;
// A flawless Zuzu's reward: this long as an adult without a single mistake
static const uint32_t SECRET_SECONDS = 432000;

static const uint8_t HEARTS_MAX = 4;
static const uint8_t DISCIPLINE_MAX = 4;
static const uint8_t POOP_MAX = 4;
static const uint8_t WEIGHT_MAX = 99;
static const uint8_t MEAL_WEIGHT = 1;
static const uint8_t SNACK_WEIGHT = 2;
static const uint8_t OVERWEIGHT = 25; // Above the form's base, a snack may make it sick
static const uint8_t SNACK_RUN_LIMIT = 3;
static const uint8_t SNACK_SICK_ODDS = 3; // One in this many, past either limit
static const uint8_t DOSES_MAX = 2;

static const uint16_t CALL_SECONDS = 900; // The original's 15 minutes to answer a call
static const uint16_t FAKE_MIN_SECONDS = 7200;
static const uint16_t FAKE_MAX_SECONDS = 18000;
static const uint32_t STARVE_SECONDS = 43200;
static const uint32_t ILLNESS_SECONDS = 43200;

static const uint8_t GAME_ROUNDS = 5;
static const uint8_t GAME_WINS_NEEDED = 3;

// The care mistakes a stage forgives, and the discipline an adult's better line asks for
static const uint8_t TEEN_GOOD_MISTAKES = 2;
static const uint8_t ADULT_GOOD_MISTAKES = 3;
static const uint8_t ADULT_FAIR_MISTAKES = 6;
static const uint8_t ADULT_GOOD_DISCIPLINE = 3;

static const uint8_t LIFE_MISTAKES_PER_DAY = 3; // Every so many mistakes costs a day of life
static const uint8_t LIFE_DAYS_MIN = 5;
static const uint8_t LIFE_DAYS_LUCK = 3;

#endif // PET_CONFIG_H
