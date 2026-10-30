// A pet's life with several kinds of owner, over many seeds. Build and run with
// tools/sim.sh [lives] [seed]. The owner acts only when they look at the board.
#include <cstdio>
#include <cstdlib>
#include "../src/pet/Pet.h"

struct Owner
{
    const char *name;
    uint32_t lookEvery; // Seconds between looks, from 8:00 to 23:00
    bool scolds;
    bool plays;
    uint8_t absentPercent; // Share of days they never look at all
    uint32_t carelessSeconds = 0; // Until then they only look twice a day
};

static const Owner OWNERS[] = {
    {"attentive", 600, true, true, 0},
    {"no scolding", 600, false, true, 0},
    {"hourly", 3600, true, true, 0},
    {"two hourly", 2 * 3600, true, true, 0},
    {"two hours lax", 2 * 3600, false, true, 0},
    {"casual", 150 * 60, true, true, 0},
    {"casual lax", 3 * 3600, false, false, 0},
    {"rare", 6 * 3600, false, true, 0},
    {"weekends off", 3600, true, true, 30},
    {"forgetful", 10 * 3600, false, false, 40},
    {"slow start", 600, true, true, 0, 26 * 3600},
};
static const uint8_t OWNER_COUNT = sizeof(OWNERS) / sizeof(OWNERS[0]);

static void care(Pet &pet, const Owner &owner)
{
    pet.act(Action::CLEAN);
    while (pet.isSick() && pet.act(Action::MEDICINE) == Refusal::NONE)
    {
    }
    if (pet.hasFakeCall() && owner.scolds)
    {
        pet.act(Action::DISCIPLINE);
    }
    if (pet.isSleeping() && pet.lightsOn())
    {
        pet.act(Action::LIGHT_OFF);
    }
    while (pet.act(Action::MEAL) == Refusal::NONE)
    {
    }
    if (owner.plays)
    {
        for (uint8_t i = 0; i < HEARTS_MAX && pet.happy() < HEARTS_MAX && pet.check(Action::PLAY) == Refusal::NONE; i++)
        {
            pet.finishGame(GAME_ROUNDS);
        }
    }
    else if (pet.happy() < 2)
    {
        pet.act(Action::SNACK);
    }
}

struct Result
{
    uint32_t forms[FORM_COUNT] = {};
    uint32_t deaths[4] = {};
    uint32_t years = 0;
    uint32_t mistakes = 0;
    uint32_t sickSeconds = 0;
    uint32_t seconds = 0;
};

static void live(const Owner &owner, uint32_t seed, Result &out)
{
    Pet pet(seed);
    Rng dice(seed * 2654435761u + 1);
    pet.setClock(9 * 3600);
    bool absent = false;
    uint32_t lastLook = 0;
    FormId last = FORM_EGG;
    for (uint32_t t = 0; t < 60 * DAY_SECONDS && !pet.isDead(); t++)
    {
        pet.tick();
        pet.takeEvents();
        uint32_t clock = pet.clockSeconds();
        if (clock == 0)
        {
            absent = dice.below(100) < owner.absentPercent;
        }
        bool awakeHours = clock >= 8 * 3600 && clock < 23 * 3600;
        uint32_t every = t < owner.carelessSeconds ? 7 * 3600 : owner.lookEvery;
        if (awakeHours && !absent && t - lastLook >= every)
        {
            lastLook = t;
            care(pet, owner);
        }
        out.seconds++;
        out.sickSeconds += pet.isSick() ? 1 : 0;
        if (pet.stage() == Stage::ADULT)
        {
            last = pet.form();
        }
    }
    out.forms[last]++;
    out.deaths[(uint8_t)pet.death()]++;
    out.years += pet.ageYears();
    out.mistakes += pet.totalMistakes();
}

int main(int argc, char **argv)
{
    uint32_t lives = argc > 1 ? (uint32_t)atoi(argv[1]) : 200;
    uint32_t seed = argc > 2 ? (uint32_t)atoi(argv[2]) : 1;
    uint32_t reached[FORM_COUNT] = {};
    printf("%-13s %5s %5s %6s %5s %5s %5s  adults\n", "owner", "age", "mist", "sick%", "negl", "ill", "old");
    for (uint8_t i = 0; i < OWNER_COUNT; i++)
    {
        Result r;
        for (uint32_t n = 0; n < lives; n++)
        {
            live(OWNERS[i], seed + n * 7919 + i, r);
        }
        printf("%-13s %5.1f %5.1f %6.1f %5u %5u %5u ", OWNERS[i].name, (double)r.years / lives,
               (double)r.mistakes / lives, 100.0 * r.sickSeconds / r.seconds, r.deaths[1], r.deaths[2], r.deaths[3]);
        for (uint8_t f = FORM_ADULT_A; f < FORM_COUNT; f++)
        {
            if (r.forms[f] > 0)
            {
                printf(" %s:%u", FORMS[f].name, r.forms[f]);
                reached[f] += r.forms[f];
            }
        }
        printf("\n");
    }
    bool all = true;
    for (uint8_t f = FORM_ADULT_A; f < FORM_COUNT; f++)
    {
        if (reached[f] == 0)
        {
            printf("Never reached: %s\n", FORMS[f].name);
            all = false;
        }
    }
    return all ? 0 : 1;
}
