// Logic tests: tools/test.sh
#include <cstdio>
#include <cstring>
#include "../src/pet/Pet.h"
#include "../src/pet/Save.h"

static int failures = 0;
#define CHECK(cond)                                                \
    do                                                             \
    {                                                              \
        if (!(cond))                                               \
        {                                                          \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);  \
            failures++;                                            \
        }                                                          \
    } while (0)

static void run(Pet &pet, uint32_t seconds)
{
    for (uint32_t i = 0; i < seconds; i++)
    {
        pet.tick();
    }
}

static void feedAll(Pet &pet)
{
    while (pet.act(Action::MEAL) == Refusal::NONE)
    {
    }
    pet.raw().happy = HEARTS_MAX;
}

static Pet hatched(uint32_t seed = 7)
{
    Pet pet(seed);
    run(pet, EGG_SECONDS);
    pet.takeEvents();
    return pet;
}

static void testHatch()
{
    Pet pet(3);
    CHECK(pet.isEgg());
    CHECK(pet.generation() == 1);
    run(pet, EGG_SECONDS - 1);
    CHECK(pet.isEgg());
    CHECK(pet.check(Action::MEAL) == Refusal::NOT_ALIVE);
    pet.tick();
    CHECK(pet.form() == FORM_BABY);
    uint16_t events = pet.takeEvents();
    CHECK(events & EVENT_HATCH);
    CHECK(events & EVENT_CALL);
    CHECK(pet.hunger() == 0 && pet.happy() == 0);
}

static void testDecay()
{
    Pet pet = hatched();
    feedAll(pet);
    CHECK(pet.hunger() == HEARTS_MAX);
    run(pet, FORMS[FORM_BABY].hungerSeconds);
    CHECK(pet.hunger() == HEARTS_MAX - 1);
}

static void testMeals()
{
    Pet pet = hatched();
    uint8_t weight = pet.weight();
    CHECK(pet.act(Action::MEAL) == Refusal::NONE);
    CHECK(pet.hunger() == 1 && pet.weight() == weight + MEAL_WEIGHT);
    feedAll(pet);
    CHECK(pet.act(Action::MEAL) == Refusal::FULL);
    weight = pet.weight();
    pet.raw().happy = 1;
    CHECK(pet.act(Action::SNACK) == Refusal::NONE);
    CHECK(pet.happy() == 2 && pet.weight() == weight + SNACK_WEIGHT);
}

static void testSnacksSicken()
{
    bool sick = false;
    for (uint32_t seed = 1; seed < 30 && !sick; seed++)
    {
        Pet pet = hatched(seed);
        for (int i = 0; i < 12; i++)
        {
            pet.act(Action::SNACK);
        }
        sick = pet.isSick();
    }
    CHECK(sick);
}

static void adult(Pet &pet, FormId form);

static void testCallCostsOneMistake()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.raw().hunger = 0;
    run(pet, CALL_SECONDS - 1);
    CHECK(pet.mistakes() == 0);
    pet.tick();
    CHECK(pet.mistakes() == 1);
    run(pet, CALL_SECONDS * 2);
    CHECK(pet.mistakes() == 1);
}

static void testCallAnswered()
{
    Pet pet = hatched();
    pet.raw().happy = HEARTS_MAX;
    run(pet, CALL_SECONDS - 10);
    pet.act(Action::MEAL);
    CHECK(!pet.needs(NEED_FOOD));
    run(pet, 100);
    CHECK(pet.mistakes() == 0);
}

static void testPoopMakesSick()
{
    Pet pet = hatched();
    pet.raw().poops = POOP_MAX - 1;
    pet.raw().poopTimer = FORMS[FORM_BABY].poopSeconds - 1;
    pet.tick();
    CHECK(pet.poops() == POOP_MAX);
    CHECK(pet.isSick());
    CHECK(pet.takeEvents() & EVENT_SICK);
    CHECK(pet.check(Action::PLAY) == Refusal::SICK);
    CHECK(pet.act(Action::CLEAN) == Refusal::NONE);
    CHECK(pet.poops() == 0);
    CHECK(pet.act(Action::CLEAN) == Refusal::NOTHING_TO_CLEAN);
    while (pet.isSick())
    {
        CHECK(pet.act(Action::MEDICINE) == Refusal::NONE);
    }
    CHECK(pet.act(Action::MEDICINE) == Refusal::NOT_SICK);
}

static void adult(Pet &pet, FormId form)
{
    pet.raw().form = form;
    pet.raw().hunger = HEARTS_MAX;
    pet.raw().happy = HEARTS_MAX;
    pet.raw().lifeDays = 100;
}

static void testSleepAndLight()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    CHECK(pet.check(Action::LIGHT_OFF) == Refusal::NOT_SLEEPY);
    pet.setClock(22 * 3600 - 2);
    pet.takeEvents();
    run(pet, 3);
    CHECK(pet.isSleeping());
    CHECK(pet.takeEvents() & EVENT_SLEEP);
    CHECK(pet.needs(NEED_DARK));
    CHECK(pet.check(Action::MEAL) == Refusal::ASLEEP);
    run(pet, CALL_SECONDS - 10);
    CHECK(pet.act(Action::LIGHT_OFF) == Refusal::NONE);
    CHECK(!pet.attention());
    run(pet, CALL_SECONDS * 2);
    CHECK(pet.mistakes() == 0);
    uint8_t hunger = pet.hunger();
    run(pet, 3 * 3600);
    CHECK(pet.hunger() == hunger);
    pet.setClock(8 * 3600 - 2);
    run(pet, 3);
    CHECK(!pet.isSleeping());
    CHECK(pet.lightsOn());
}

static void testLightLeftOnCosts()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.setClock(22 * 3600 - 1);
    run(pet, CALL_SECONDS + 2);
    CHECK(pet.mistakes() == 1);
}

static void testClockChangeNeverPunishes()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.setClock(10 * 3600);
    uint32_t age = pet.ageYears();
    uint32_t stage = pet.raw().stageSeconds;
    pet.setClock(23 * 3600);
    CHECK(pet.isSleeping());
    CHECK(!pet.lightsOn());
    CHECK(!pet.attention());
    pet.takeEvents();
    run(pet, 1);
    CHECK(pet.mistakes() == 0);
    pet.setClock(3 * 3600);
    CHECK(pet.isSleeping());
    pet.setClock(12 * 3600);
    CHECK(!pet.isSleeping());
    CHECK(pet.lightsOn());
    CHECK(pet.ageYears() == age);
    CHECK(pet.raw().stageSeconds == stage + 1);
    CHECK(pet.mistakes() == 0);
    CHECK(!(pet.takeEvents() & EVENT_DEATH));
}

static void testClockDoesNotCutASleepCallShort()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.setClock(22 * 3600 - 1);
    run(pet, CALL_SECONDS - 1);
    pet.setClock(23 * 3600);
    CHECK(pet.isSleeping());
    run(pet, 5);
    CHECK(pet.mistakes() == 0);
}

static void testFakeCall()
{
    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.raw().fakeSeconds = 1;
    CHECK(pet.attention());
    CHECK(pet.check(Action::MEAL) == Refusal::TANTRUM);
    CHECK(pet.act(Action::DISCIPLINE) == Refusal::NONE);
    CHECK(pet.discipline() == 1);
    CHECK(!pet.hasFakeCall());
    uint8_t happy = pet.happy();
    CHECK(pet.act(Action::DISCIPLINE) == Refusal::NO_REASON);
    CHECK(pet.happy() == happy - 1);
    CHECK(pet.discipline() == 1);
    pet.raw().fakeSeconds = 1;
    run(pet, CALL_SECONDS + 1);
    CHECK(!pet.hasFakeCall());
    CHECK(pet.mistakes() == 0);
}

static void testGame()
{
    Pet pet = hatched();
    pet.raw().weight = 12;
    pet.raw().happy = 1;
    pet.finishGame(GAME_WINS_NEEDED);
    CHECK(pet.happy() == 2 && pet.weight() == 11);
    pet.finishGame(GAME_WINS_NEEDED - 1);
    CHECK(pet.happy() == 2 && pet.weight() == 10);
}

static FormId evolveTeen(uint8_t teen, uint8_t mistakes, uint8_t discipline)
{
    Pet pet = hatched();
    adult(pet, (FormId)teen);
    pet.raw().mistakes = mistakes;
    pet.raw().discipline = discipline;
    pet.raw().stageSeconds = TEEN_SECONDS - 1;
    pet.setClock(12 * 3600);
    pet.tick();
    return pet.form();
}

static void testEvolution()
{
    CHECK(evolveTeen(FORM_TEEN_A, 0, 4) == FORM_ADULT_A);
    CHECK(evolveTeen(FORM_TEEN_A, 0, 0) == FORM_ADULT_B);
    CHECK(evolveTeen(FORM_TEEN_A, 4, 4) == FORM_ADULT_B);
    CHECK(evolveTeen(FORM_TEEN_A, 7, 4) == FORM_ADULT_C);
    CHECK(evolveTeen(FORM_TEEN_B, 0, 4) == FORM_ADULT_D);
    CHECK(evolveTeen(FORM_TEEN_B, 5, 4) == FORM_ADULT_E);
    CHECK(evolveTeen(FORM_TEEN_B, 9, 0) == FORM_ADULT_F);

    Pet pet = hatched();
    adult(pet, FORM_ADULT_A);
    pet.raw().stageSeconds = SECRET_SECONDS - 1;
    pet.setClock(12 * 3600);
    pet.tick();
    CHECK(pet.form() == FORM_SECRET);

    Pet sloppy = hatched();
    adult(sloppy, FORM_ADULT_A);
    sloppy.raw().mistakes = 1;
    sloppy.raw().stageSeconds = SECRET_SECONDS - 1;
    sloppy.setClock(12 * 3600);
    sloppy.tick();
    CHECK(sloppy.form() == FORM_ADULT_A);
}

static void testGrowthWaitsForMorning()
{
    Pet pet = hatched();
    adult(pet, FORM_TEEN_A);
    pet.raw().stageSeconds = TEEN_SECONDS + 5;
    pet.setClock(23 * 3600);
    run(pet, 10);
    CHECK(pet.form() == FORM_TEEN_A);
}

static void testLifeCost()
{
    Pet careful = hatched();
    adult(careful, FORM_TEEN_A);
    careful.raw().discipline = 4;
    careful.raw().stageSeconds = TEEN_SECONDS - 1;
    careful.setClock(12 * 3600);
    careful.tick();
    CHECK(careful.lifeDays() >= FORMS[FORM_ADULT_A].lifeDays);

    Pet sloppy = hatched();
    adult(sloppy, FORM_TEEN_B);
    sloppy.raw().totalMistakes = 200;
    sloppy.raw().stageSeconds = TEEN_SECONDS - 1;
    sloppy.setClock(12 * 3600);
    sloppy.tick();
    CHECK(sloppy.lifeDays() == LIFE_DAYS_MIN);
}

static void starve(Pet &pet)
{
    adult(pet, FORM_ADULT_A);
    pet.raw().hunger = 0;
    pet.setClock(9 * 3600);
    run(pet, STARVE_SECONDS);
}

static void testDeaths()
{
    Pet starved = hatched();
    adult(starved, FORM_ADULT_A);
    starved.raw().hunger = 0;
    starved.setClock(9 * 3600);
    run(starved, STARVE_SECONDS - 1);
    CHECK(!starved.isDead());
    starved.tick();
    CHECK(starved.death() == Death::NEGLECT);
    CHECK(starved.takeEvents() & EVENT_DEATH);
    CHECK(starved.check(Action::MEAL) == Refusal::NOT_ALIVE);

    Pet fed = hatched();
    adult(fed, FORM_ADULT_A);
    fed.raw().doses = 1;
    fed.raw().sickSeconds = ILLNESS_SECONDS - 2;
    fed.setClock(12 * 3600);
    run(fed, 2);
    CHECK(fed.death() == Death::ILLNESS);

    Pet old = hatched();
    adult(old, FORM_ADULT_A);
    old.raw().lifeDays = 1;
    old.raw().ageSeconds = DAY_SECONDS * 2;
    old.setClock(23 * 3600);
    CHECK(old.isSleeping());
    run(old, 3600);
    CHECK(!old.isDead());
    old.setClock(8 * 3600 - 1);
    old.raw().sleeping = 1;
    run(old, 2);
    CHECK(old.death() == Death::OLD_AGE);
}

static void testNewEgg()
{
    Pet pet = hatched();
    pet.raw().ageSeconds = DAY_SECONDS * 6;
    starve(pet);
    CHECK(pet.isDead());
    uint16_t best = pet.bestYears();
    CHECK(best >= 6);
    pet.setClock(9 * 3600);
    pet.newEgg();
    CHECK(pet.isEgg());
    CHECK(!pet.isDead());
    CHECK(pet.generation() == 2);
    CHECK(pet.bestYears() == best);
    CHECK(pet.clockSeconds() == 9 * 3600);
}

static void testFrozenWhileDead()
{
    Pet pet = hatched();
    starve(pet);
    uint16_t age = pet.ageYears();
    run(pet, DAY_SECONDS * 2);
    CHECK(pet.ageYears() == age);
}

static void testSaveRoundTrip()
{
    Pet pet = hatched(11);
    feedAll(pet);
    run(pet, 5000);
    SlotStore store("boba-test");
    SaveBlob blob;
    captureSave(blob, pet);
    CHECK(writeSave(store, blob));
    SaveBlob read;
    CHECK(readSave(store, read));
    Pet back(99);
    back.load(read.pet);
    CHECK(back.hunger() == pet.hunger());
    CHECK(back.clockSeconds() == pet.clockSeconds());
    run(pet, 20000);
    run(back, 20000);
    CHECK(back.hunger() == pet.hunger());
    CHECK(back.poops() == pet.poops());
    CHECK(back.raw().fakeTimer == pet.raw().fakeTimer);
    CHECK(back.raw().doses == pet.raw().doses);

    blob.version++;
    writeSave(store, blob);
    CHECK(!readSave(store, read));
}

int main()
{
    testHatch();
    testDecay();
    testMeals();
    testSnacksSicken();
    testCallCostsOneMistake();
    testCallAnswered();
    testPoopMakesSick();
    testSleepAndLight();
    testLightLeftOnCosts();
    testClockChangeNeverPunishes();
    testClockDoesNotCutASleepCallShort();
    testFakeCall();
    testGame();
    testEvolution();
    testGrowthWaitsForMorning();
    testLifeCost();
    testDeaths();
    testNewEgg();
    testFrozenWhileDead();
    testSaveRoundTrip();
    printf(failures == 0 ? "All tests passed\n" : "%d failed\n", failures);
    return failures != 0;
}
