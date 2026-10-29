#ifndef PET_PET_H
#define PET_PET_H

#include <stdint.h>
#include "Config.h"
#include "Rng.h"

enum class Mood : uint8_t
{
    ASLEEP,
    SICK,
    SLEEPY,
    SAD,
    MEH,
    CONTENT,
    HAPPY,
    COUNT
};

enum class Action : uint8_t
{
    MEAL,
    SNACK,
    CUDDLE,
    CLEAN,
    LIGHT,
    MEDS,
    WARM,
    COUNT
};

// Why an action was refused, for the screen to say so
enum class Refusal : uint8_t
{
    NONE,
    EGG,     // only Warm works on an egg
    NOT_EGG, // and Warm only on one
    ASLEEP,
    FULL,
    NO_FLUFF,
    NOT_SICK,
    TIRED, // too sleepy to play
};

// Bits returned by takeEvents(), for sounds and notices
enum PetEvent : uint16_t
{
    EV_HATCHED = 1 << 0,
    EV_EVOLVED = 1 << 1,
    EV_SLEPT = 1 << 2,
    EV_WOKE = 1 << 3,
    EV_SICK = 1 << 4,
    EV_CURED = 1 << 5,
    EV_FLUFF = 1 << 6,
    EV_CALL = 1 << 7, // a need just ran low; lastCall() says which
};

// The pet's whole state, advanced one second of powered time per tick(). Plain C++ with no
// Arduino include, so tools/ run it on the PC.
class Pet
{
public:
    // POD, saved raw: primitives and indices into the config tables only
    struct Snapshot
    {
        char name[NAME_LEN + 1];
        uint8_t form;
        uint8_t needs[NEED_COUNT];
        uint16_t needTimer[NEED_COUNT]; // seconds toward the next point
        uint8_t fluff;
        uint8_t fluffSpot[MAX_FLUFF];
        uint8_t flags;
        uint8_t snackHeat;
        uint8_t callMask; // needs that called and have not been seen to since
        uint16_t snackTimer;
        uint16_t fluffTimer;
        uint16_t lowSecs;
        uint16_t hourTimer;
        uint16_t cuddleCooldown;
        uint16_t sleepSecs; // in this sleep
        uint16_t bond;
        uint16_t bondPeak;
        uint16_t careFood; // meals and snacks in this stage: what decides the next form
        uint16_t carePlay; // minigames
        uint32_t age;       // since hatching
        uint32_t stageSecs; // in this form
    };

    explicit Pet(uint32_t seed = 1) : rng(seed) { newEgg(); }

    void newEgg();
    void tick();

    Refusal check(Action action) const;
    // True when it was done; otherwise check() says why
    bool act(Action action);
    Refusal checkPlay() const;
    // After a minigame: joy (clamped to PLAY_MIN_JOY..PLAY_MAX_JOY) and food
    void playReward(uint8_t joy, uint8_t food);
    void setName(const char *name);
    void pickDefaultName();

    uint16_t takeEvents();
    Need lastCall() const { return calledNeed; }

    Mood mood() const;
    const char *getName() const { return state.name; }
    FormId getForm() const { return (FormId)state.form; }
    Stage getStage() const { return FORMS[state.form].stage; }
    bool isEgg() const { return getStage() == STAGE_EGG; }
    uint8_t getNeed(Need need) const { return state.needs[need]; }
    // 0..NEED_MAX like the needs: each fluff takes a third of it
    uint8_t getTidy() const { return (uint8_t)(NEED_MAX - (uint16_t)state.fluff * NEED_MAX / MAX_FLUFF); }
    uint8_t getFluff() const { return state.fluff; }
    uint8_t getFluffSpot(uint8_t i) const { return state.fluffSpot[i]; }
    bool isAsleep() const { return state.flags & FLAG_ASLEEP; }
    bool isLightOn() const { return !(state.flags & FLAG_DARK); }
    bool isSick() const { return state.flags & FLAG_SICK; }
    uint16_t getBond() const { return state.bond; }
    uint32_t getAge() const { return state.age; }
    uint32_t getStageSecs() const { return state.stageSecs; }
    // Seconds until it grows, 0 for an adult
    uint32_t getSecsToGrow() const;
    bool canLeaveEgg() const;

    void save(Snapshot &out) const { out = state; }
    void load(const Snapshot &in);
    void reseed(uint32_t seed) { rng = Rng(seed); }

#ifdef PET_TOOLS
    Snapshot &raw() { return state; }
#endif

private:
    static const uint8_t FLAG_ASLEEP = 1 << 0;
    static const uint8_t FLAG_DARK = 1 << 1;
    static const uint8_t FLAG_SICK = 1 << 2;

    Snapshot state;
    Rng rng;
    uint16_t events = 0;
    Need calledNeed = NEED_FOOD;

    void raise(Need need, uint8_t amount);
    void lower(Need need, uint8_t amount);
    void careFor(uint8_t before);
    void gainBond(uint8_t amount);
    void loseBond(uint8_t amount);
    void setFlag(uint8_t flag, bool on);
    void tickNeeds();
    void tickFluff();
    void tickHealth();
    void tickGrowth();
    void fallAsleep();
    void wake();
    void becomeForm(FormId form);
    uint8_t lowCount() const;
    void resetFluffTimer();
};

#endif // PET_PET_H
