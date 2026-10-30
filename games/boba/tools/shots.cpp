// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/pet/Pet.h"
#include "../src/screens/ClockScreen.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/GameScreen.h"
#include "../src/screens/HomeScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/ui/Panel.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static TFT_eSPI tft;
static Pet pet(42);
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static LcdMatrix lcd;
static Panel panel(lcd);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "The pet, the best age and the", "settings will be erased.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static ClockScreen clockScreen(pet, lcd, panel, nothing, screens, sound);
static GameScreen gameScreen(pet, lcd, panel, nothing, screens, sound);
static HomeScreen home(pet, lcd, panel, nothing, screens, sound, gameScreen);
static SettingsScreen settingsScreen(settings, applyTheme, nothing, screens, sound, resetScreen, clockScreen);

static std::string outDir = ".";

static void run(unsigned long ms)
{
    for (unsigned long t = 0; t < ms; t += 20)
    {
        stubNow += 20;
        uint16_t events = pet.takeEvents();
        if (events != 0 && screens.getCurrentScreen() == &home)
        {
            home.handleEvents(events);
        }
        screens.update(stubNow);
    }
}

static void shot(const std::string &name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

static void reenter()
{
    screens.openModal(&settingsScreen);
    screens.closeModal();
}

static void setForm(FormId form)
{
    Pet::Snapshot &s = pet.raw();
    s.form = form;
    s.hunger = 3;
    s.happy = 2;
    s.weight = FORMS[form].baseWeight + 3;
    s.discipline = 2;
    s.doses = 0;
    s.poops = 0;
    s.sleeping = 0;
    s.lightsOn = 1;
    s.death = 0;
    s.lifeDays = 30;
    s.ageSeconds = 4 * DAY_SECONDS + 100;
    s.stageSeconds = 0;
    s.clockSeconds = 10 * 3600;
    s.fakeSeconds = 0;
    pet.takeEvents();
    reenter();
    screens.handleUpPress();
}

static void select(int8_t icon)
{
    for (int8_t i = 0; i <= icon; i++)
    {
        screens.handleRightPress();
    }
}

static void back()
{
    screens.handleUpPress();
    screens.handleUpPress();
    run(30);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    srand(3);
    screens.addScreen(&home, "Boba");
    screens.openModal(&clockScreen);
    run(100);
    shot("01_clock");
    screens.handleConfirmPress();
    run(50);
    shot("02_clock_minutes");
    screens.handleConfirmPress();
    run(100);
    shot("03_egg");

    for (uint32_t i = 0; i < EGG_SECONDS; i++)
    {
        pet.tick();
    }
    run(1200);
    shot("04_hatching");
    run(2500);
    shot("05_baby_calling");

    for (uint8_t f = FORM_BABY; f < FORM_COUNT; f++)
    {
        setForm((FormId)f);
        run(700);
        char name[32];
        snprintf(name, sizeof(name), "forms_%02u_%s", f, FORMS[f].name);
        shot(name);
    }

    setForm(FORM_ADULT_A);
    select(ICON_FOOD);
    run(30);
    shot("06_selected");
    screens.handleConfirmPress();
    run(30);
    shot("07_food_menu");
    screens.handleConfirmPress();
    run(1600);
    shot("08_eating");
    run(3000);

    setForm(FORM_ADULT_B);
    select(ICON_METER);
    screens.handleConfirmPress();
    run(30);
    shot("09_meter_age");
    screens.handleConfirmPress();
    run(30);
    shot("10_meter_hungry");
    screens.handleConfirmPress();
    screens.handleConfirmPress();
    run(30);
    shot("11_meter_discipline");
    back();

    setForm(FORM_TEEN_A);
    pet.raw().poops = 3;
    pet.raw().doses = 1;
    reenter();
    run(800);
    shot("12_sick_and_dirty");

    pet.raw().poops = 3;
    home.handleEvents(0);
    select(ICON_BATH);
    screens.handleConfirmPress();
    run(1300);
    shot("13_flush");
    run(5000);
    back();

    setForm(FORM_ADULT_D);
    select(ICON_MEDICINE);
    pet.raw().doses = 1;
    screens.handleConfirmPress();
    run(1300);
    shot("14_medicine");
    run(6000);
    back();

    setForm(FORM_ADULT_C);
    pet.raw().fakeSeconds = 1;
    select(ICON_DISCIPLINE);
    screens.handleConfirmPress();
    run(200);
    shot("15_scold");
    run(3000);
    back();

    setForm(FORM_ADULT_A);
    pet.setClock(23 * 3600);
    pet.raw().lightsOn = 1;
    reenter();
    run(700);
    shot("16_asleep_light_on");
    pet.act(Action::LIGHT_OFF);
    run(700);
    shot("17_lights_off");

    setForm(FORM_ADULT_A);
    screens.handleDownPress();
    run(50);
    shot("18_clock");
    screens.handleDownPress();

    setForm(FORM_ADULT_E);
    select(ICON_GAME);
    screens.handleConfirmPress();
    run(50);
    shot("19_game_ask");
    screens.handleBackPress();
    run(100);
    shot("20_game_reveal");
    for (int i = 0; i < 4; i++)
    {
        run(1600);
        screens.handleRightPress();
    }
    run(1500);
    shot("21_game_score");
    run(3000);

    pet.raw().ageSeconds = 4 * DAY_SECONDS;
    setForm(FORM_TEEN_B);
    pet.raw().stageSeconds = TEEN_SECONDS - 1;
    pet.raw().mistakes = 1;
    pet.raw().discipline = 4;
    pet.tick();
    run(1100);
    shot("22_evolving");
    run(4000);

    setForm(FORM_ADULT_A);
    pet.raw().hunger = 0;
    pet.raw().starveSeconds = STARVE_SECONDS - 1;
    pet.tick();
    run(800);
    shot("23_dying");
    run(5000);
    shot("24_grave");
    screens.handleConfirmPress();
    run(50);
    shot("25_grave_stats");
    screens.handleConfirmPress();
    run(600);

    setForm(FORM_ADULT_A);
    pet.raw().poops = 2;
    pet.raw().hunger = 0;
    reenter();
    for (uint8_t t = 0; t < THEME_COUNT; t++)
    {
        settings.theme = t;
        applyTheme();
        screens.repaint();
        reenter();
        run(700);
        shot(std::string("theme_") + THEMES[t].name);
    }
    settings.theme = 0;
    applyTheme();
    reenter();

    screens.openModal(&settingsScreen);
    run(50);
    shot("26_settings");
    screens.closeModal();
    return 0;
}
