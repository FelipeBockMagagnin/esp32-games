// Tests on the PC: the rasterizer, the forms and the text, a pet's life, the cozy promise and
// the save.
// Build and run with tools/test.sh.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <Preferences.h>
#include <cmath>
#include "../src/art/Rig.h"
#include "../src/art/Room.h"
#include "../src/art/Spring.h"
#include "../src/gfx/Canvas.h"
#include "../src/gfx/Font.h"
#include "../src/pet/Album.h"
#include "../src/pet/Pet.h"
#include "../src/pet/Save.h"
#include "../src/screens/Lines.h"

static int failures = 0;
#define CHECK(cond, ...)                                                                                              \
    do                                                                                                                \
    {                                                                                                                 \
        if (!(cond))                                                                                                  \
        {                                                                                                             \
            failures++;                                                                                               \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                               \
            printf(__VA_ARGS__);                                                                                      \
            printf("\n");                                                                                             \
        }                                                                                                             \
    } while (0)

static bool printable(const char *text)
{
    for (const char *p = text; *p != '\0'; p++)
    {
        if (*p < 32 || *p > 126)
        {
            return false;
        }
    }
    return true;
}

// A shape's pixels against its exact anti-aliased cover: the row spans must not change them
static void testCanvas()
{
    static uint16_t band[Canvas::W * Canvas::BAND_H];
    int worst = 0;
    srand(1);
    for (int trial = 0; trial < 300; trial++)
    {
        float cx = 20 + rand() % 200 + (rand() % 100) / 100.0f, cy = rand() % 32 + (rand() % 100) / 100.0f;
        float rx = 1 + rand() % 60 + (rand() % 100) / 100.0f, ry = 1 + rand() % 40 + (rand() % 100) / 100.0f;
        Canvas c;
        c.begin(band, 0);
        c.clear(0);
        c.fillEllipse(cx, cy, rx, ry, 0xFFFF);
        for (int y = 0; y < Canvas::BAND_H; y++)
        {
            for (int x = 0; x < Canvas::W; x++)
            {
                float ux = (x + 0.5f - cx) / rx, uy = (y + 0.5f - cy) / ry;
                float k = sqrtf(ux * ux + uy * uy), gx = ux / rx, gy = uy / ry, g = sqrtf(gx * gx + gy * gy);
                float d = g > 1e-6f ? (k - 1) * k / g : -rx;
                float cover = d >= 0.5f ? 0 : d <= -0.5f ? 1 : 0.5f - d;
                int diff = abs((int)(cover * 31 + 0.5f) - (band[y * Canvas::W + x] >> 11));
                worst = diff > worst ? diff : worst;
            }
        }
    }
    // The blend's own rounding is worth a couple of steps; a missed edge pixel is worth many
    CHECK(worst <= 3, "an ellipse's spans differ from its exact cover by %d of 31", worst);
}

// However slow the board's frames get, a kicked spring settles instead of running away
static void testSprings()
{
    for (float dt : {0.016f, 0.05f, 0.1f, 0.15f, 0.25f})
    {
        Spring pop(320, 16, 1), stretch(190, 13);
        float worst = 0;
        for (int i = 0; i < 200; i++)
        {
            if (i % 10 == 0)
            {
                pop.kick(6);
                stretch.kick(-5);
            }
            pop.step(dt);
            stretch.step(dt);
            worst = fmaxf(worst, fmaxf(fabsf(pop.value - 1), fabsf(stretch.value)));
        }
        CHECK(worst < 0.6f, "a spring runs away at %.0f ms frames (%.2f)", dt * 1000, worst);
    }
}

static void testArt()
{
    static uint16_t band[Canvas::W * Canvas::BAND_H], again[Canvas::W * Canvas::BAND_H];
    for (uint8_t f = 0; f < FORM_COUNT; f++)
    {
        // The biggest it gets on Home: stretched by the spring, lifted by a jump, at the room's edge
        Pose p;
        p.ground = Room::GROUND;
        p.squashY = 1.25f;
        p.squashX = 1.15f;
        p.lift = 30;
        p.puff = 1;
        for (float x : {Room::WALK_LEFT + FORM_RIG[f].w * 0.5f, Room::WALK_RIGHT - FORM_RIG[f].w * 0.5f})
        {
            p.x = x;
            float x0, y0, x1, y1;
            petBounds((FormId)f, p, x0, y0, x1, y1);
            CHECK(x0 >= Room::X && x1 <= Room::X + Room::W && y0 >= Room::Y,
                  "%s leaves the room at x %.0f (%.0f..%.0f, top %.0f)", FORMS[f].name, x, x0, x1, y0);
        }
        CHECK(FORM_RIG[f].w * 0.5f * 1.15f <= (Room::WALK_RIGHT - Room::WALK_LEFT) / 2.0f, "%s is too wide to walk",
              FORMS[f].name);
        // draw() runs once per band: twice the same band must give the same pixels
        for (int b = 0; b < 2; b++)
        {
            Canvas c;
            uint16_t *out = b == 0 ? band : again;
            c.begin(out, Room::GROUND - 40);
            c.clear(0x1234);
            p.x = 120;
            p.lift = 0;
            drawPet(c, (FormId)f, p);
        }
        CHECK(memcmp(band, again, sizeof(band)) == 0, "%s draws differently twice", FORMS[f].name);
    }
}

static void testText()
{
    char line[64];
    for (uint8_t m = 0; m < (uint8_t)Mood::COUNT; m++)
    {
        for (uint8_t i = 0; i < LINES_PER_MOOD; i++)
        {
            snprintf(line, sizeof(line), MOOD_LINES[m][i], "Wwwwwwww");
            CHECK(printable(line) && Canvas::textWidth(line, FONT_SMALL) <= 210, "mood line: %s", line);
        }
    }
    for (const char *fmt : ACTION_LINES)
    {
        snprintf(line, sizeof(line), fmt, "Wwwwwwww");
        CHECK(printable(line) && Canvas::textWidth(line, FONT_SMALL) <= 210, "action line: %s", line);
    }
    for (const char *fmt : REFUSAL_LINES)
    {
        snprintf(line, sizeof(line), fmt, "Wwwwwwww");
        CHECK(printable(line) && Canvas::textWidth(line, FONT_SMALL) <= 210, "refusal line: %s", line);
    }
    for (const FormDef &form : FORMS)
    {
        CHECK(strlen(form.name) <= NAME_LEN, "form name %s", form.name);
        CHECK(printable(form.note) && Canvas::textWidth(form.note, FONT_BODY) <= 200, "form note %s", form.note);
    }
    for (const char *name : DEFAULT_NAMES)
    {
        CHECK(strlen(name) <= NAME_LEN, "default name %s", name);
    }
}

static void live(Pet &pet, uint32_t secs)
{
    for (uint32_t i = 0; i < secs; i++)
    {
        pet.tick();
    }
}

static void testGrowth()
{
    Pet pet(3);
    CHECK(pet.isEgg(), "a new pet is an egg");
    CHECK(!pet.act(Action::MEAL), "an egg does not eat");
    live(pet, EGG_SECS);
    CHECK(pet.getForm() == FORM_BLIP && (pet.takeEvents() & EV_HATCHED), "the egg hatches after EGG_SECS");

    // Played with more than fed: a Tuft
    for (int i = 0; i < 5; i++)
    {
        pet.playReward(40, 0);
    }
    pet.act(Action::MEAL);
    pet.raw().needs[NEED_REST] = NEED_MAX;
    live(pet, BABY_SECS);
    CHECK(pet.getForm() == FORM_TUFT, "a played-with baby grows into a Tuft, not %s", FORMS[pet.getForm()].name);

    pet.raw().bond = ADULT_HIGH_BOND;
    pet.raw().needs[NEED_REST] = NEED_MAX;
    pet.raw().stageSecs = CHILD_SECS - 1;
    pet.tick();
    CHECK(pet.getForm() == FORM_NIMBUS, "a close Tuft grows into a Nimbus, not %s", FORMS[pet.getForm()].name);

    Pet fed(4);
    live(fed, EGG_SECS);
    fed.act(Action::SNACK);
    fed.raw().stageSecs = BABY_SECS - 1;
    fed.tick();
    CHECK(fed.getForm() == FORM_MOCHI, "a fed baby grows into a Mochi");
    fed.raw().bond = 10;
    fed.raw().needs[NEED_REST] = NEED_MAX;
    fed.raw().stageSecs = CHILD_SECS - 1;
    fed.tick();
    CHECK(fed.getForm() == FORM_PEBBLE, "a distant Mochi grows into a Pebble");

    // Asleep, it waits to wake before growing
    Pet sleepy(5);
    live(sleepy, EGG_SECS);
    sleepy.raw().needs[NEED_REST] = 10;
    sleepy.act(Action::LIGHT);
    CHECK(sleepy.isAsleep(), "the lamp off puts a tired pet to bed");
    sleepy.raw().stageSecs = BABY_SECS;
    sleepy.tick();
    CHECK(sleepy.getForm() == FORM_BLIP, "it does not grow in its sleep");
    live(sleepy, 4000);
    CHECK(sleepy.isAsleep(), "rested in the dark, it sleeps on through the night");
    sleepy.act(Action::LIGHT);
    sleepy.tick();
    CHECK(!sleepy.isAsleep(), "the lamp on wakes a rested pet");
    CHECK(sleepy.getForm() != FORM_BLIP, "and it grows once awake");

    Pet forgotten(6);
    live(forgotten, EGG_SECS);
    forgotten.raw().needs[NEED_REST] = 10;
    forgotten.act(Action::LIGHT);
    live(forgotten, DARK_SLEEP_MAX_SECS + 1);
    CHECK(!forgotten.isAsleep() && forgotten.isLightOn(), "left in the dark, it wakes itself and turns the lamp on");
}

static Pet worstPet()
{
    Pet pet(9);
    live(pet, EGG_SECS);
    Pet::Snapshot &raw = pet.raw();
    for (auto &need : raw.needs)
    {
        need = 0;
    }
    raw.needs[NEED_REST] = NEED_MAX;
    raw.fluff = MAX_FLUFF;
    raw.flags |= 1 << 2;
    return pet;
}

static void testCozy()
{
    // Each trouble has one action that lifts it out of Sad
    {
        Pet pet = worstPet();
        CHECK(pet.act(Action::MEDS) && !pet.isSick(), "Meds cures at once");
        CHECK(pet.act(Action::MEAL) && pet.getNeed(NEED_FOOD) >= NEED_LOW, "one meal ends hunger");
        CHECK(pet.act(Action::CLEAN) && pet.getFluff() == 0, "one sweep tidies the room");
        CHECK(pet.act(Action::CUDDLE) && pet.getNeed(NEED_JOY) >= NEED_LOW, "one cuddle ends sadness");
        CHECK(pet.mood() != Mood::SAD && pet.mood() != Mood::SICK, "four actions from the worst state: no longer sad");
    }
    {
        Pet pet = worstPet();
        pet.playReward(0, 0);
        CHECK(pet.getNeed(NEED_JOY) >= NEED_LOW, "even a lost minigame ends sadness");
    }
    // A month untouched: still there, never past its bounds, the bond at most halved
    Pet pet(11);
    live(pet, EGG_SECS);
    for (int i = 0; i < 20; i++)
    {
        pet.act(Action::CUDDLE);
        pet.raw().cuddleCooldown = 0;
    }
    uint16_t peak = pet.getBond();
    live(pet, 30 * 86400);
    for (uint8_t i = 0; i < NEED_COUNT; i++)
    {
        CHECK(pet.getNeed((Need)i) <= NEED_MAX, "need %u out of bounds", i);
    }
    CHECK(pet.getBond() >= peak / 2, "the bond never drops below half its peak (%u of %u)", pet.getBond(), peak);
    CHECK(pet.getFluff() <= MAX_FLUFF, "fluff stays capped");

    Pet snacker(12);
    live(snacker, EGG_SECS);
    for (uint8_t i = 0; i < SNACK_LIMIT; i++)
    {
        snacker.act(Action::SNACK);
    }
    CHECK(snacker.isSick(), "too many snacks give a tummy ache");
}

static void testSave()
{
    Preferences::store.clear();
    SlotStore store("test");
    Pet pet(13);
    live(pet, EGG_SECS + 100);
    pet.setName("Tester");
    Album album;
    album.clear();
    album.see(pet.getForm());
    SaveBlob blob, read;
    captureSave(blob, pet, album);
    CHECK(writeSave(store, blob), "save written");
    SlotStore again("test");
    CHECK(readSave(again, read) && memcmp(&blob, &read, sizeof(blob)) == 0, "save read back");

    live(pet, 50);
    SaveBlob newer;
    captureSave(newer, pet, album);
    Preferences::failWrites = true;
    CHECK(!writeSave(again, newer), "a failed write reports it");
    Preferences::failWrites = false;
    SlotStore third("test");
    CHECK(readSave(third, read) && memcmp(&blob, &read, sizeof(blob)) == 0, "a torn write keeps the last save");

    Pet restored;
    restored.load(read.pet);
    CHECK(strcmp(restored.getName(), "Tester") == 0, "the name survives");

    blob.version++;
    writeSave(third, blob);
    SlotStore fourth("test");
    CHECK(!readSave(fourth, read), "a save of another version is discarded");

    album.farewell(pet);
    CHECK(pet.isEgg() && album.generation == 1 && strcmp(album.past[0].name, "Tester") == 0,
          "passing on leaves a memory and an egg");
}

int main()
{
    testCanvas();
    testSprings();
    testArt();
    testText();
    testGrowth();
    testCozy();
    testSave();
    if (failures == 0)
    {
        printf("all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
