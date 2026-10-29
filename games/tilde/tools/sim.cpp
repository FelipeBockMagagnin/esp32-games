// Plays a pet's first days with a few kinds of owner and prints how it grew and how it felt,
// to tune src/pet/Config.h. Build and run with tools/sim.sh [days] [seed].
#include <cstdio>
#include <cstdlib>
#include "../src/pet/Pet.h"

struct Owner
{
    const char *name;
    const char *about;
    // Waking hours (of a day starting at midnight) when it looks at the pet
    uint32_t checkMask;
    bool plays;
    int awayFromDay; // -1 never; otherwise it leaves the board alone for two days from then
};

static const uint32_t EVERY_WAKING_HOUR = 0xFFFF00; // 08:00-23:00
static const Owner OWNERS[] = {
    {"attentive", "every waking hour, plays", EVERY_WAKING_HOUR, true, -1},
    {"feeder", "every waking hour, snacks, no play", EVERY_WAKING_HOUR, false, -1},
    {"busy", "08, 13, 19 and 23 h", (1u << 8) | (1u << 13) | (1u << 19) | (1u << 23), true, -1},
    {"weekend", "attentive, away days 2-3", EVERY_WAKING_HOUR, true, 1},
    {"absent", "never looks", 0, false, -1},
};

static void checkIn(Pet &pet, const Owner &owner, int hour)
{
    if (pet.isEgg())
    {
        for (int i = 0; i < 40; i++)
        {
            pet.act(Action::WARM);
        }
        return;
    }
    // Morning, or a nap in the dark that is over: the lamp goes back on
    if (!pet.isLightOn() && (hour == 8 || (hour < 23 && pet.getNeed(NEED_REST) >= NEED_MAX)))
    {
        pet.act(Action::LIGHT);
        pet.tick();
    }
    if (pet.isSick())
    {
        pet.act(Action::MEDS);
    }
    if (!owner.plays && (pet.getNeed(NEED_JOY) < 85 || pet.getNeed(NEED_FOOD) < 85))
    {
        pet.act(Action::SNACK);
    }
    if (pet.getNeed(NEED_FOOD) < 60)
    {
        pet.act(Action::MEAL);
    }
    if (pet.getFluff() > 0)
    {
        pet.act(Action::CLEAN);
    }
    if (owner.plays && pet.getNeed(NEED_JOY) < 85 && pet.checkPlay() == Refusal::NONE)
    {
        pet.playReward(45, 0);
    }
    if (pet.getNeed(NEED_JOY) < 80)
    {
        pet.act(Action::CUDDLE);
    }
    if (!pet.isAsleep() && pet.getNeed(NEED_REST) < REST_BEDTIME && pet.isLightOn())
    {
        pet.act(Action::LIGHT);
    }
    else if (hour == 23 && pet.isLightOn() && !pet.isAsleep())
    {
        pet.act(Action::LIGHT);
    }
}

static void run(const Owner &owner, int days, uint32_t seed)
{
    Pet pet(seed);
    uint32_t moodSecs[(uint8_t)Mood::COUNT] = {};
    uint32_t sickSecs = 0, total = 0;
    char grew[3][48] = {"", "", ""};
    int grown = 0;
    // The board is switched on at 08:00
    for (uint32_t t = 8 * 3600; t < (uint32_t)days * 86400 + 8 * 3600; t++)
    {
        int day = t / 86400, hour = (t / 3600) % 24;
        bool away = owner.awayFromDay >= 0 && day >= owner.awayFromDay && day < owner.awayFromDay + 2;
        if (t % 3600 == 0 && (owner.checkMask & (1u << hour)) && !away)
        {
            checkIn(pet, owner, hour);
        }
        pet.tick();
        uint16_t events = pet.takeEvents();
        if ((events & (EV_HATCHED | EV_EVOLVED)) && grown < 3)
        {
            snprintf(grew[grown++], sizeof(grew[0]), "%s d%d %02d:%02d b%u", FORMS[pet.getForm()].name, day, hour,
                     (int)(t / 60 % 60), (unsigned)pet.getBond());
        }
        if (!pet.isEgg())
        {
            moodSecs[(uint8_t)pet.mood()]++;
            sickSecs += pet.isSick() ? 1 : 0;
            total++;
        }
    }
    printf("%-10s %-30s\n", owner.name, owner.about);
    printf("           grew: %-22s %-22s %-22s\n", grew[0], grew[1], grew[2]);
    printf("           bond %u   sick %4.1f%%   ", (unsigned)pet.getBond(), 100.0 * sickSecs / (total ? total : 1));
    static const char *const MOODS[] = {"asleep", "sick", "sleepy", "sad", "meh", "content", "happy"};
    for (uint8_t m = 0; m < (uint8_t)Mood::COUNT; m++)
    {
        printf("%s %2.0f%%  ", MOODS[m], 100.0 * moodSecs[m] / (total ? total : 1));
    }
    printf("\n\n");
}

int main(int argc, char **argv)
{
    int days = argc > 1 ? atoi(argv[1]) : 3;
    uint32_t seed = argc > 2 ? (uint32_t)atoi(argv[2]) : 1;
    for (const Owner &owner : OWNERS)
    {
        run(owner, days, seed);
    }
    return 0;
}
