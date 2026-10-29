#include "Pet.h"
#include <string.h>

void Pet::newEgg()
{
    memset(&state, 0, sizeof(state));
    state.form = FORM_EGG;
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        state.needs[i] = NEED_MAX;
    }
    state.hourTimer = 3600;
    resetFluffTimer();
    pickDefaultName();
}

void Pet::load(const Snapshot &in)
{
    state = in;
    state.name[NAME_LEN] = '\0';
    if (state.form >= FORM_COUNT)
    {
        newEgg();
    }
    if (state.hourTimer == 0)
    {
        state.hourTimer = 3600;
    }
}

void Pet::pickDefaultName()
{
    setName(DEFAULT_NAMES[rng.below(DEFAULT_NAME_COUNT)]);
}

void Pet::setName(const char *name)
{
    memset(state.name, 0, sizeof(state.name));
    strncpy(state.name, name, NAME_LEN);
}

uint16_t Pet::takeEvents()
{
    uint16_t out = events;
    events = 0;
    return out;
}

void Pet::setFlag(uint8_t flag, bool on)
{
    state.flags = on ? (state.flags | flag) : (state.flags & ~flag);
}

void Pet::resetFluffTimer()
{
    state.fluffTimer = FLUFF_EVERY_SECS - FLUFF_JITTER_SECS + rng.below(2 * FLUFF_JITTER_SECS + 1);
}

void Pet::raise(Need need, uint8_t amount)
{
    uint16_t value = state.needs[need] + amount;
    state.needs[need] = value > NEED_MAX ? NEED_MAX : (uint8_t)value;
    if (state.needs[need] >= NEED_OK)
    {
        state.callMask &= ~(1 << need);
    }
}

void Pet::lower(Need need, uint8_t amount)
{
    state.needs[need] = state.needs[need] > amount ? state.needs[need] - amount : 0;
    if (state.needs[need] < NEED_LOW && !(state.callMask & (1 << need)) && !isAsleep())
    {
        state.callMask |= 1 << need;
        calledNeed = need;
        events |= EV_CALL;
    }
}

void Pet::gainBond(uint8_t amount)
{
    if (isSick())
    {
        amount = (amount + 1) / 2;
    }
    uint16_t value = state.bond + amount;
    state.bond = value > BOND_MAX ? BOND_MAX : value;
    if (state.bond > state.bondPeak)
    {
        state.bondPeak = state.bond;
    }
}

void Pet::loseBond(uint8_t amount)
{
    uint16_t floor = state.bondPeak / 2;
    state.bond = state.bond > floor + amount ? state.bond - amount : floor;
}

void Pet::careFor(uint8_t before)
{
    gainBond(before < NEED_OK ? BOND_CARE : BOND_TOUCH);
}

uint8_t Pet::lowCount() const
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        // Rest is mending while it sleeps
        if (state.needs[i] < NEED_LOW && !(i == NEED_REST && isAsleep()))
        {
            count++;
        }
    }
    return count + (state.fluff >= MAX_FLUFF ? 1 : 0);
}

Mood Pet::mood() const
{
    if (isEgg())
    {
        return Mood::CONTENT;
    }
    if (isAsleep())
    {
        return Mood::ASLEEP;
    }
    if (isSick())
    {
        return Mood::SICK;
    }
    if (state.needs[NEED_REST] < NEED_LOW)
    {
        return Mood::SLEEPY;
    }
    if (lowCount() > 0)
    {
        return Mood::SAD;
    }
    uint8_t least = NEED_MAX;
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        least = state.needs[i] < least ? state.needs[i] : least;
    }
    if (least < NEED_OK || state.fluff >= 2)
    {
        return Mood::MEH;
    }
    return least >= NEED_HIGH && state.fluff == 0 ? Mood::HAPPY : Mood::CONTENT;
}

void Pet::tick()
{
    if (isEgg())
    {
        tickGrowth();
        return;
    }
    state.age++;
    tickNeeds();
    tickFluff();
    tickHealth();
    tickGrowth();
}

void Pet::tickNeeds()
{
    bool asleep = isAsleep();
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        uint16_t every;
        if (i == NEED_REST && asleep)
        {
            every = isLightOn() ? REST_REFILL_LIT : REST_REFILL_DARK;
        }
        else
        {
            every = asleep ? NEEDS[i].asleepSecsPerPoint : NEEDS[i].awakeSecsPerPoint;
            if (i == NEED_JOY && isSick())
            {
                every /= 2;
            }
        }
        if (every == 0 || ++state.needTimer[i] < every)
        {
            continue;
        }
        state.needTimer[i] = 0;
        if (i == NEED_REST && asleep)
        {
            raise(NEED_REST, 1);
        }
        else
        {
            lower((Need)i, 1);
        }
    }

    if (asleep && state.sleepSecs < 0xFFFF)
    {
        state.sleepSecs++;
    }
    if (asleep && state.needs[NEED_REST] >= NEED_MAX && (isLightOn() || state.sleepSecs >= DARK_SLEEP_MAX_SECS))
    {
        wake();
    }
    else if (!asleep && (state.needs[NEED_REST] == 0 || (!isLightOn() && state.needs[NEED_REST] < REST_BEDTIME)))
    {
        fallAsleep();
    }

    if (state.cuddleCooldown > 0)
    {
        state.cuddleCooldown--;
    }
    if (state.snackHeat > 0 && ++state.snackTimer >= SNACK_COOL_SECS)
    {
        state.snackTimer = 0;
        state.snackHeat--;
    }
}

void Pet::tickFluff()
{
    if (isAsleep() || state.fluff >= MAX_FLUFF)
    {
        return;
    }
    if (state.fluffTimer > 0)
    {
        state.fluffTimer--;
        return;
    }
    state.fluffSpot[state.fluff++] = (uint8_t)rng.below(ROOM_SPOTS);
    resetFluffTimer();
    events |= EV_FLUFF;
}

void Pet::tickHealth()
{
    uint8_t lows = lowCount();
    if (isAsleep())
    {
        // Nothing worsens in its sleep
    }
    else if (lows >= 2 && !isSick())
    {
        if (++state.lowSecs >= SICK_AFTER_SECS)
        {
            setFlag(FLAG_SICK, true);
            events |= EV_SICK;
        }
    }
    else if (lows < 2)
    {
        state.lowSecs = 0;
    }

    if (--state.hourTimer == 0)
    {
        state.hourTimer = 3600;
        if (lows >= 2)
        {
            loseBond(BOND_NEGLECT);
        }
        else
        {
            bool cared = !isSick();
            for (uint8_t i = 0; i < NEED_COUNT; i++)
            {
                cared = cared && state.needs[i] >= NEED_OK;
            }
            if (cared)
            {
                gainBond(BOND_HOURLY);
            }
        }
    }
}

uint32_t Pet::getSecsToGrow() const
{
    uint32_t span;
    switch (getStage())
    {
    case STAGE_EGG:
        span = EGG_SECS;
        break;
    case STAGE_BABY:
        span = BABY_SECS;
        break;
    case STAGE_CHILD:
        span = CHILD_SECS;
        break;
    default:
        return 0;
    }
    return state.stageSecs < span ? span - state.stageSecs : 0;
}

void Pet::tickGrowth()
{
    state.stageSecs++;
    // A sleeping pet grows when it wakes, so the change is seen
    if (getStage() == STAGE_ADULT || isAsleep() || getSecsToGrow() > 0)
    {
        return;
    }
    switch (getForm())
    {
    case FORM_EGG:
        becomeForm(FORM_BLIP);
        events |= EV_HATCHED;
        return;
    case FORM_BLIP:
        becomeForm(state.carePlay > state.careFood ? FORM_TUFT : FORM_MOCHI);
        break;
    case FORM_MOCHI:
        becomeForm(state.bond >= ADULT_HIGH_BOND ? FORM_PUDDLE : FORM_PEBBLE);
        break;
    case FORM_TUFT:
        becomeForm(state.bond >= ADULT_HIGH_BOND ? FORM_NIMBUS : FORM_SPROUT);
        break;
    default:
        return;
    }
    events |= EV_EVOLVED;
}

void Pet::becomeForm(FormId form)
{
    state.form = form;
    state.stageSecs = 0;
    state.careFood = 0;
    state.carePlay = 0;
}

void Pet::fallAsleep()
{
    setFlag(FLAG_ASLEEP, true);
    state.sleepSecs = 0;
    events |= EV_SLEPT;
}

void Pet::wake()
{
    setFlag(FLAG_ASLEEP, false);
    // It switches the lamp back on itself
    setFlag(FLAG_DARK, false);
    events |= EV_WOKE;
}

bool Pet::canLeaveEgg() const
{
    return getStage() == STAGE_ADULT && state.stageSecs >= LEAVE_EGG_ADULT_SECS && state.bond >= LEAVE_EGG_BOND;
}

Refusal Pet::check(Action action) const
{
    if (action == Action::WARM)
    {
        return isEgg() ? Refusal::NONE : Refusal::NOT_EGG;
    }
    if (isEgg())
    {
        return Refusal::EGG;
    }
    switch (action)
    {
    case Action::MEAL:
        return isAsleep() ? Refusal::ASLEEP : state.needs[NEED_FOOD] >= FULL_FOOD ? Refusal::FULL : Refusal::NONE;
    case Action::SNACK:
    case Action::CUDDLE:
        return isAsleep() ? Refusal::ASLEEP : Refusal::NONE;
    case Action::CLEAN:
        return state.fluff == 0 ? Refusal::NO_FLUFF : Refusal::NONE;
    case Action::MEDS:
        return isSick() ? Refusal::NONE : Refusal::NOT_SICK;
    default:
        return Refusal::NONE;
    }
}

bool Pet::act(Action action)
{
    if (check(action) != Refusal::NONE)
    {
        return false;
    }
    uint8_t food = state.needs[NEED_FOOD], joy = state.needs[NEED_JOY];
    switch (action)
    {
    case Action::MEAL:
        raise(NEED_FOOD, MEAL_FOOD);
        careFor(food);
        state.careFood++;
        break;
    case Action::SNACK:
        raise(NEED_FOOD, SNACK_FOOD);
        raise(NEED_JOY, SNACK_JOY);
        gainBond(BOND_TOUCH);
        state.careFood++;
        state.snackTimer = 0;
        if (++state.snackHeat >= SNACK_LIMIT && !isSick())
        {
            setFlag(FLAG_SICK, true);
            events |= EV_SICK;
        }
        break;
    case Action::CUDDLE:
        if (state.cuddleCooldown == 0)
        {
            raise(NEED_JOY, CUDDLE_JOY);
            careFor(joy);
            state.cuddleCooldown = CUDDLE_COOLDOWN_SECS;
        }
        else
        {
            raise(NEED_JOY, CUDDLE_REPEAT_JOY);
        }
        break;
    case Action::CLEAN:
        gainBond(state.fluff >= 2 ? BOND_CARE : BOND_TOUCH);
        state.fluff = 0;
        resetFluffTimer();
        break;
    case Action::LIGHT:
        setFlag(FLAG_DARK, isLightOn());
        if (!isAsleep() && !isLightOn() && state.needs[NEED_REST] < REST_BEDTIME)
        {
            fallAsleep();
            gainBond(state.needs[NEED_REST] < NEED_LOW ? BOND_CARE : BOND_TOUCH);
        }
        break;
    case Action::MEDS:
        setFlag(FLAG_SICK, false);
        state.lowSecs = 0;
        state.snackHeat = 0;
        gainBond(BOND_TOUCH);
        events |= EV_CURED;
        break;
    case Action::WARM:
        state.stageSecs += WARM_SECS;
        break;
    default:
        return false;
    }
    return true;
}

Refusal Pet::checkPlay() const
{
    if (isEgg())
    {
        return Refusal::EGG;
    }
    if (isAsleep())
    {
        return Refusal::ASLEEP;
    }
    return state.needs[NEED_REST] < PLAY_MIN_REST ? Refusal::TIRED : Refusal::NONE;
}

void Pet::playReward(uint8_t joy, uint8_t food)
{
    joy = joy < PLAY_MIN_JOY ? PLAY_MIN_JOY : joy > PLAY_MAX_JOY ? PLAY_MAX_JOY : joy;
    uint8_t before = state.needs[NEED_JOY];
    raise(NEED_JOY, joy);
    raise(NEED_FOOD, food);
    lower(NEED_REST, PLAY_REST_COST);
    careFor(before);
    state.carePlay++;
}
