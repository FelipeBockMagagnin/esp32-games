#ifndef PET_PET_H
#define PET_PET_H

#include <stdint.h>
#include "Config.h"
#include "Rng.h"

enum class Action : uint8_t
{
    MEAL,
    SNACK,
    LIGHT_ON,
    LIGHT_OFF,
    PLAY,
    MEDICINE,
    CLEAN,
    DISCIPLINE
};

enum class Refusal : uint8_t
{
    NONE,
    NOT_ALIVE, // An egg, or the grave
    ASLEEP,
    FULL,
    TANTRUM, // A false call: it refuses until scolded
    SICK,
    NOT_SICK,
    NOT_SLEEPY,
    NOTHING_TO_CLEAN,
    NO_REASON // Scolded for nothing, which act() still applies
};

enum class Death : uint8_t
{
    NONE,
    NEGLECT,
    ILLNESS,
    OLD_AGE
};

enum Need : uint8_t
{
    NEED_FOOD,
    NEED_FUN,
    NEED_CURE,
    NEED_DARK,
    NEED_COUNT
};

enum PetEvent : uint16_t
{
    EVENT_HATCH = 1 << 0,
    EVENT_EVOLVE = 1 << 1,
    EVENT_CALL = 1 << 2,
    EVENT_SLEEP = 1 << 3,
    EVENT_WAKE = 1 << 4,
    EVENT_SICK = 1 << 5,
    EVENT_POOP = 1 << 6,
    EVENT_DEATH = 1 << 7
};

// The pet's whole state, advanced by tick() once per powered second. Time only passes through
// tick(): with the board off the pet is frozen, and setClock() is never counted as elapsed.
class Pet
{
public:
    // POD, saved as raw bytes: primitives and table indices only
    struct Snapshot
    {
        uint32_t ageSeconds;
        uint32_t stageSeconds;
        uint32_t clockSeconds;
        uint32_t rngState;
        uint32_t starveSeconds;
        uint32_t sickSeconds;
        uint32_t bestAgeSeconds;
        uint16_t generation;
        uint16_t hungerTimer;
        uint16_t happyTimer;
        uint16_t poopTimer;
        uint16_t fakeTimer;
        uint16_t fakeSeconds; // 0 when there is no false call
        uint16_t callTimers[NEED_COUNT];
        uint8_t callsCounted; // Need bits whose mistake this call already cost
        uint8_t form;
        uint8_t hunger;
        uint8_t happy;
        uint8_t discipline;
        uint8_t weight;
        uint8_t poops;
        uint8_t doses;
        uint8_t snackRun;
        uint8_t mistakes;      // This stage's
        uint8_t totalMistakes;
        uint8_t lifeDays;
        uint8_t sleeping;
        uint8_t lightsOn;
        uint8_t death; // Death
    };

    explicit Pet(uint32_t seed = 1);

    // Today's fresh egg; generation and the best age carry over
    void newEgg();
    void tick();

    Refusal check(Action action) const;
    Refusal act(Action action); // Does nothing unless check() passes, bar NO_REASON
    void finishGame(uint8_t wins);
    bool rollGameDirection() { return rng.below(2) == 0; }

    // Puts the pet where the new hour says it should be, without punishing the change
    void setClock(uint32_t secondsOfDay);

    uint16_t takeEvents();

    const FormDef &def() const { return FORMS[s.form]; }
    FormId form() const { return (FormId)s.form; }
    Stage stage() const { return def().stage; }
    bool isEgg() const { return s.form == FORM_EGG; }
    bool isDead() const { return s.death != (uint8_t)Death::NONE; }
    Death death() const { return (Death)s.death; }
    bool isSleeping() const { return s.sleeping != 0; }
    bool lightsOn() const { return s.lightsOn != 0; }
    bool isSick() const { return s.doses > 0; }
    bool hasFakeCall() const { return s.fakeSeconds > 0; }
    bool needs(Need need) const;
    bool attention() const;

    uint8_t hunger() const { return s.hunger; }
    uint8_t happy() const { return s.happy; }
    uint8_t discipline() const { return s.discipline; }
    uint8_t weight() const { return s.weight; }
    uint8_t poops() const { return s.poops; }
    uint8_t mistakes() const { return s.mistakes; }
    uint8_t totalMistakes() const { return s.totalMistakes; }
    uint8_t lifeDays() const { return s.lifeDays; }
    uint16_t ageYears() const { return (uint16_t)(s.ageSeconds / DAY_SECONDS); }
    uint16_t bestYears() const { return (uint16_t)(s.bestAgeSeconds / DAY_SECONDS); }
    uint16_t generation() const { return s.generation; }
    uint32_t clockSeconds() const { return s.clockSeconds; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

#ifdef PET_TOOLS
    Snapshot &raw() { return s; }
#endif

private:
    Snapshot s;
    Rng rng;
    uint16_t events = 0;
    bool wasCalling = false;

    bool inSleepHours() const;
    void becomeForm(FormId form);
    void hatch();
    void die(Death cause);
    void makeSick();
    void addMistake();
    void wake();
    void updateSleep();
    void decay();
    void updateFakeCall();
    void updateCalls();
    void updateDeath();
    void updateGrowth();
    void armFakeTimer();
    void clearCall(Need need);
    void noteCalling();
};

#endif // PET_PET_H
