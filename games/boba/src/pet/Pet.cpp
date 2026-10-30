#include "Pet.h"
#include <string.h>

Pet::Pet(uint32_t seed) : rng(seed)
{
    memset(&s, 0, sizeof(s));
    s.clockSeconds = 12 * 3600;
    s.generation = 0;
    newEgg();
}

void Pet::newEgg()
{
    Snapshot next;
    memset(&next, 0, sizeof(next));
    next.clockSeconds = s.clockSeconds;
    next.bestAgeSeconds = s.bestAgeSeconds;
    next.generation = s.generation + 1;
    next.form = FORM_EGG;
    next.weight = FORMS[FORM_EGG].baseWeight;
    next.lightsOn = 1;
    s = next;
    wasCalling = false;
}

void Pet::becomeForm(FormId form)
{
    s.form = form;
    s.stageSeconds = 0;
    s.mistakes = 0;
    if (s.weight < def().baseWeight)
    {
        s.weight = def().baseWeight;
    }
}

// The original hatches hungry and bored, so the first thing a new owner hears is a call
void Pet::hatch()
{
    becomeForm(FORM_BABY);
    armFakeTimer();
    events |= EVENT_HATCH;
}

void Pet::armFakeTimer()
{
    s.fakeTimer = (uint16_t)rng.range(FAKE_MIN_SECONDS, FAKE_MAX_SECONDS);
}

void Pet::die(Death cause)
{
    s.death = (uint8_t)cause;
    s.sleeping = 0;
    s.lightsOn = 1;
    s.fakeSeconds = 0;
    if (s.ageSeconds > s.bestAgeSeconds)
    {
        s.bestAgeSeconds = s.ageSeconds;
    }
    events |= EVENT_DEATH;
}

void Pet::makeSick()
{
    if (s.doses == 0)
    {
        s.doses = (uint8_t)rng.range(1, DOSES_MAX);
        events |= EVENT_SICK;
    }
}

void Pet::addMistake()
{
    if (s.mistakes < 255)
    {
        s.mistakes++;
    }
    if (s.totalMistakes < 255)
    {
        s.totalMistakes++;
    }
}

bool Pet::needs(Need need) const
{
    if (isEgg() || isDead())
    {
        return false;
    }
    switch (need)
    {
    case NEED_FOOD:
        return !isSleeping() && s.hunger == 0;
    case NEED_FUN:
        return !isSleeping() && s.happy == 0;
    case NEED_CURE:
        return !isSleeping() && s.doses > 0;
    case NEED_DARK:
        return isSleeping() && s.lightsOn;
    default:
        return false;
    }
}

bool Pet::attention() const
{
    if (hasFakeCall())
    {
        return true;
    }
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        if (needs((Need)i))
        {
            return true;
        }
    }
    return false;
}

bool Pet::inSleepHours() const
{
    const FormDef &f = def();
    if (f.sleepHour == f.wakeHour)
    {
        return false;
    }
    uint8_t hour = (uint8_t)(s.clockSeconds / 3600);
    return f.sleepHour < f.wakeHour ? hour >= f.sleepHour && hour < f.wakeHour
                                    : hour >= f.sleepHour || hour < f.wakeHour;
}

void Pet::wake()
{
    s.sleeping = 0;
    s.lightsOn = 1;
    clearCall(NEED_DARK);
    events |= EVENT_WAKE;
}

void Pet::updateSleep()
{
    bool due = inSleepHours();
    if (due && !isSleeping())
    {
        s.sleeping = 1;
        s.fakeSeconds = 0;
        events |= EVENT_SLEEP;
    }
    else if (!due && isSleeping())
    {
        wake();
        if (stage() == Stage::ADULT && s.ageSeconds >= (uint32_t)s.lifeDays * DAY_SECONDS)
        {
            die(Death::OLD_AGE);
        }
    }
}

void Pet::decay()
{
    const FormDef &f = def();
    if (++s.hungerTimer >= f.hungerSeconds)
    {
        s.hungerTimer = 0;
        if (s.hunger > 0)
        {
            s.hunger--;
        }
    }
    if (++s.happyTimer >= f.happySeconds)
    {
        s.happyTimer = 0;
        if (s.happy > 0)
        {
            s.happy--;
        }
    }
    if (++s.poopTimer >= f.poopSeconds)
    {
        s.poopTimer = 0;
        if (s.poops < POOP_MAX)
        {
            s.poops++;
            events |= EVENT_POOP;
        }
        if (s.poops >= POOP_MAX)
        {
            makeSick();
        }
    }
}

void Pet::updateFakeCall()
{
    if (s.fakeSeconds > 0)
    {
        if (++s.fakeSeconds > CALL_SECONDS)
        {
            s.fakeSeconds = 0;
        }
        return;
    }
    if (stage() < Stage::CHILD || --s.fakeTimer > 0)
    {
        if (s.fakeTimer == 0)
        {
            armFakeTimer();
        }
        return;
    }
    armFakeTimer();
    bool real = false;
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        real = real || needs((Need)i);
    }
    if (!real && rng.below(DISCIPLINE_MAX + 1) >= s.discipline)
    {
        s.fakeSeconds = 1;
    }
}

void Pet::clearCall(Need need)
{
    s.callTimers[need] = 0;
    s.callsCounted &= (uint8_t)~(1 << need);
}

void Pet::updateCalls()
{
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        if (!needs((Need)i))
        {
            if (i == NEED_DARK || !isSleeping())
            {
                clearCall((Need)i);
            }
            continue;
        }
        if (s.callsCounted & (1 << i))
        {
            continue;
        }
        if (++s.callTimers[i] >= CALL_SECONDS)
        {
            s.callsCounted |= (uint8_t)(1 << i);
            addMistake();
        }
    }
}

void Pet::updateDeath()
{
    if (isSleeping())
    {
        return;
    }
    s.starveSeconds = (s.hunger == 0 || s.happy == 0) ? s.starveSeconds + 1 : 0;
    s.sickSeconds = s.doses > 0 ? s.sickSeconds + 1 : 0;
    if (s.starveSeconds >= STARVE_SECONDS)
    {
        die(Death::NEGLECT);
    }
    else if (s.sickSeconds >= ILLNESS_SECONDS)
    {
        die(Death::ILLNESS);
    }
}

void Pet::updateGrowth()
{
    FormId next = FORM_COUNT;
    switch (s.form)
    {
    case FORM_BABY:
        if (s.stageSeconds >= BABY_SECONDS)
        {
            next = FORM_CHILD;
        }
        break;
    case FORM_CHILD:
        if (s.stageSeconds >= CHILD_SECONDS)
        {
            next = s.mistakes <= TEEN_GOOD_MISTAKES ? FORM_TEEN_A : FORM_TEEN_B;
        }
        break;
    case FORM_TEEN_A:
    case FORM_TEEN_B:
        if (s.stageSeconds >= TEEN_SECONDS)
        {
            uint8_t line = s.form == FORM_TEEN_A ? FORM_ADULT_A : FORM_ADULT_D;
            if (s.mistakes > ADULT_FAIR_MISTAKES)
            {
                next = (FormId)(line + 2);
            }
            else if (s.mistakes > ADULT_GOOD_MISTAKES || s.discipline < ADULT_GOOD_DISCIPLINE)
            {
                next = (FormId)(line + 1);
            }
            else
            {
                next = (FormId)line;
            }
        }
        break;
    case FORM_ADULT_A:
        if (s.stageSeconds >= SECRET_SECONDS && s.mistakes == 0)
        {
            next = FORM_SECRET;
        }
        break;
    default:
        break;
    }
    if (next == FORM_COUNT)
    {
        return;
    }
    becomeForm(next);
    if (stage() == Stage::ADULT)
    {
        int16_t days = FORMS[next].lifeDays + rng.below(LIFE_DAYS_LUCK + 1) - s.totalMistakes / LIFE_MISTAKES_PER_DAY;
        s.lifeDays = (uint8_t)(days < LIFE_DAYS_MIN ? LIFE_DAYS_MIN : days);
    }
    events |= EVENT_EVOLVE;
}

void Pet::noteCalling()
{
    bool calling = attention();
    if (calling && !wasCalling)
    {
        events |= EVENT_CALL;
    }
    wasCalling = calling;
}

void Pet::tick()
{
    s.clockSeconds = (s.clockSeconds + 1) % DAY_SECONDS;
    if (isDead())
    {
        return;
    }
    s.stageSeconds++;
    if (isEgg())
    {
        if (s.stageSeconds >= EGG_SECONDS)
        {
            hatch();
            noteCalling();
        }
        return;
    }
    s.ageSeconds++;
    updateSleep();
    if (!isDead() && !isSleeping())
    {
        decay();
        updateFakeCall();
    }
    if (!isDead())
    {
        updateCalls();
        updateDeath();
    }
    if (!isDead() && !isSleeping())
    {
        updateGrowth();
    }
    noteCalling();
}

Refusal Pet::check(Action action) const
{
    if (isEgg() || isDead())
    {
        return Refusal::NOT_ALIVE;
    }
    if (action == Action::LIGHT_ON)
    {
        return Refusal::NONE;
    }
    if (action == Action::LIGHT_OFF)
    {
        return isSleeping() ? Refusal::NONE : Refusal::NOT_SLEEPY;
    }
    if (isSleeping())
    {
        return Refusal::ASLEEP;
    }
    switch (action)
    {
    case Action::MEAL:
        if (hasFakeCall())
        {
            return Refusal::TANTRUM;
        }
        return s.hunger >= HEARTS_MAX ? Refusal::FULL : Refusal::NONE;
    case Action::SNACK:
    case Action::PLAY:
        if (hasFakeCall())
        {
            return Refusal::TANTRUM;
        }
        return isSick() ? Refusal::SICK : Refusal::NONE;
    case Action::MEDICINE:
        return isSick() ? Refusal::NONE : Refusal::NOT_SICK;
    case Action::CLEAN:
        return s.poops > 0 ? Refusal::NONE : Refusal::NOTHING_TO_CLEAN;
    case Action::DISCIPLINE:
        return hasFakeCall() ? Refusal::NONE : Refusal::NO_REASON;
    default:
        return Refusal::NONE;
    }
}

Refusal Pet::act(Action action)
{
    Refusal refusal = check(action);
    if (refusal == Refusal::NO_REASON)
    {
        if (s.happy > 0)
        {
            s.happy--;
        }
        return refusal;
    }
    if (refusal != Refusal::NONE)
    {
        return refusal;
    }
    switch (action)
    {
    case Action::MEAL:
        s.hunger++;
        s.hungerTimer = 0;
        s.snackRun = 0;
        s.weight = (uint8_t)(s.weight + MEAL_WEIGHT > WEIGHT_MAX ? WEIGHT_MAX : s.weight + MEAL_WEIGHT);
        break;
    case Action::SNACK:
        if (s.happy < HEARTS_MAX)
        {
            s.happy++;
        }
        s.happyTimer = 0;
        s.snackRun++;
        s.weight = (uint8_t)(s.weight + SNACK_WEIGHT > WEIGHT_MAX ? WEIGHT_MAX : s.weight + SNACK_WEIGHT);
        if ((s.snackRun > SNACK_RUN_LIMIT || s.weight > def().baseWeight + OVERWEIGHT) &&
            rng.below(SNACK_SICK_ODDS) == 0)
        {
            makeSick();
        }
        break;
    case Action::LIGHT_ON:
        s.lightsOn = 1;
        break;
    case Action::LIGHT_OFF:
        s.lightsOn = 0;
        break;
    case Action::MEDICINE:
        s.doses--;
        break;
    case Action::CLEAN:
        s.poops = 0;
        break;
    case Action::DISCIPLINE:
        s.fakeSeconds = 0;
        if (s.discipline < DISCIPLINE_MAX)
        {
            s.discipline++;
        }
        break;
    default:
        break;
    }
    return Refusal::NONE;
}

void Pet::finishGame(uint8_t wins)
{
    if (isEgg() || isDead())
    {
        return;
    }
    if (wins >= GAME_WINS_NEEDED && s.happy < HEARTS_MAX)
    {
        s.happy++;
        s.happyTimer = 0;
    }
    if (s.weight > def().baseWeight)
    {
        s.weight--;
    }
}

void Pet::setClock(uint32_t secondsOfDay)
{
    s.clockSeconds = secondsOfDay % DAY_SECONDS;
    if (isEgg() || isDead())
    {
        return;
    }
    bool due = inSleepHours();
    if (due != isSleeping())
    {
        s.sleeping = due ? 1 : 0;
        s.lightsOn = due ? 0 : 1;
        s.fakeSeconds = 0;
    }
    clearCall(NEED_DARK);
    wasCalling = attention();
}

uint16_t Pet::takeEvents()
{
    uint16_t taken = events;
    events = 0;
    return taken;
}

void Pet::save(Snapshot &out) const
{
    memcpy(&out, &s, sizeof(s));
    out.rngState = rng.getState();
}

void Pet::load(const Snapshot &in)
{
    s = in;
    rng = Rng(in.rngState);
    wasCalling = attention();
}
