#include "HomeScreen.h"
#include <stdio.h>
#include "../ui/PetDraw.h"

static const unsigned long STEP_MS = 500;
static const unsigned long SCENE_STEP_MS = 350;
static const unsigned long SELECTION_MS = 20000;
static const unsigned long CLOCK_MS = 5000;
static const uint8_t METER_PAGES = 5;
static const int16_t POOP_X = 18;
static const int16_t POOP_PITCH = 7;

static const uint8_t SCENE_STEPS[] = {0, 8, 8, 4, 6, 14, 8, 6, 8, 10, 12};

HomeScreen::HomeScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
                       SoundManager &sound, Screen &gameScreen)
    : pet(pet),
      lcd(lcd),
      panel(panel),
      changed(changed),
      screenManager(screenManager),
      sound(sound),
      gameScreen(gameScreen)
{
}

void HomeScreen::onEnter(TFT_eSPI &tft)
{
    panel.paint(tft);
    shownForm = pet.form();
    mode = pet.isDead() ? Mode::GRAVE : Mode::PET;
    scene = Scene::NONE;
    draw();
    render(tft);
}

void HomeScreen::input()
{
    lastInputAt = millis();
}

void HomeScreen::update(unsigned long now)
{
    if (mode == Mode::CLOCK && (long)(now - clockEndsAt) >= 0)
    {
        mode = Mode::PET;
    }
    if (selected != Panel::NONE && mode == Mode::PET && now - lastInputAt >= SELECTION_MS)
    {
        selected = Panel::NONE;
    }
    if ((long)(now - nextStepAt) >= 0)
    {
        nextStepAt = now + (mode == Mode::SCENE ? SCENE_STEP_MS : STEP_MS);
        step(now);
    }
}

void HomeScreen::render(TFT_eSPI &tft)
{
    panel.setIcons(tft, selected, pet.attention());
    lcd.present(tft);
}

void HomeScreen::handleEvents(uint16_t events)
{
    if (events & EVENT_DEATH)
    {
        selected = Panel::NONE;
        startScene(Scene::DEATH);
    }
    else if (events & EVENT_HATCH)
    {
        startScene(Scene::HATCH);
    }
    else if (events & EVENT_EVOLVE)
    {
        startScene(Scene::EVOLVE);
    }
    else if ((events & (EVENT_SLEEP | EVENT_WAKE)) && mode != Mode::SCENE)
    {
        mode = Mode::PET;
    }
    draw();
}

void HomeScreen::startScene(Scene next)
{
    scene = next;
    sceneStep = 0;
    mode = Mode::SCENE;
    nextStepAt = millis() + SCENE_STEP_MS;
}

void HomeScreen::endScene()
{
    Scene ended = scene;
    scene = Scene::NONE;
    shownForm = pet.form();
    mode = ended == Scene::DEATH ? Mode::GRAVE : Mode::PET;
    graveStep = 0;
    if (ended == Scene::INJECT && !pet.isSick())
    {
        startScene(Scene::CHEER);
    }
}

void HomeScreen::step(unsigned long now)
{
    second = !second;
    if (mode == Mode::SCENE)
    {
        stepScene();
    }
    else if (mode == Mode::PET)
    {
        stepWander();
    }
    draw();
}

void HomeScreen::stepWander()
{
    if (pet.isEgg() || pet.isSleeping() || pet.isSick())
    {
        return;
    }
    const Sprite &art = FORM_ART[pet.form()];
    int16_t maxX = (pet.poops() > 0 ? POOP_X - 1 : LcdMatrix::COLS) - art.w;
    switch (random(4))
    {
    case 0:
        facingLeft = !facingLeft;
        break;
    case 1:
        break;
    default:
        petX += facingLeft ? -2 : 2;
        break;
    }
    if (petX <= 0 || petX >= maxX)
    {
        petX = petX <= 0 ? 0 : (maxX > 0 ? maxX : 0);
        facingLeft = petX > 0;
    }
}

void HomeScreen::stepScene()
{
    if (++sceneStep >= SCENE_STEPS[(uint8_t)scene])
    {
        endScene();
    }
}

void HomeScreen::draw()
{
    lcd.clear();
    lcd.setInverted(mode == Mode::PET && pet.isSleeping() && !pet.lightsOn());
    switch (mode)
    {
    case Mode::PET:
        drawPet();
        break;
    case Mode::FOOD_MENU:
        drawMenu("MEAL", "SNACK");
        break;
    case Mode::LIGHT_MENU:
        drawMenu("ON", "OFF");
        break;
    case Mode::METER:
        drawMeter();
        break;
    case Mode::CLOCK:
        drawClock();
        break;
    case Mode::SCENE:
        drawScene();
        break;
    case Mode::GRAVE:
        drawGrave();
        break;
    }
}

void HomeScreen::drawPetAt(int16_t x, Pose pose)
{
    const Sprite &art = FORM_ART[shownForm];
    lcd.spriteOn(art, x, LcdMatrix::ROWS, facingLeft, pose);
}

void HomeScreen::drawPet()
{
    Pose pose;
    pose.second = second;
    if (pet.isEgg())
    {
        const Sprite &art = FORM_ART[FORM_EGG];
        lcd.spriteOn(art, (LcdMatrix::COLS - art.w) / 2 + (second ? 1 : 0), LcdMatrix::ROWS, false, pose);
        return;
    }
    pose.eyesShut = pet.isSleeping() || pet.isSick();
    pose.second = pet.isSleeping() ? false : second;
    const Sprite &art = FORM_ART[pet.form()];
    bool crowded = pet.isSleeping() || pet.poops() > 0;
    int16_t x = pet.isSleeping() || pet.isSick() ? (crowded ? 1 : (LcdMatrix::COLS - art.w) / 2) : petX;
    shownForm = pet.form();
    drawPetAt(x, pose);
    drawExtras();
}

void HomeScreen::drawPoop(uint8_t index)
{
    lcd.sprite(ART_POOP, POOP_X + (index % 2) * POOP_PITCH, LcdMatrix::ROWS - ART_POOP.h - (index / 2) * POOP_PITCH);
}

void HomeScreen::drawExtras()
{
    for (uint8_t i = 0; i < pet.poops(); i++)
    {
        drawPoop(i);
    }
    if (pet.isSleeping())
    {
        int16_t y = second ? 0 : 1;
        lcd.text(LcdMatrix::COLS - 4, y, "Z");
        lcd.text(LcdMatrix::COLS - 9, y + 3, "Z");
    }
    else if (pet.isSick())
    {
        // With the upper poops there too, the two take turns
        if (pet.poops() <= 2 || second)
        {
            lcd.fill(POOP_X, 0, LcdMatrix::COLS - POOP_X, LcdMatrix::ROWS - ART_POOP.h, false);
            lcd.sprite(ART_SKULL, LcdMatrix::COLS - 8, 0);
        }
    }
}

void HomeScreen::drawMenu(const char *first, const char *other)
{
    lcd.text(1, 1, menuRow == 0 ? ">" : " ");
    lcd.text(6, 1, first);
    lcd.text(1, 9, menuRow == 1 ? ">" : " ");
    lcd.text(6, 9, other);
}

void HomeScreen::drawMeter()
{
    char line[12];
    switch (meterPage)
    {
    case 0:
        lcd.textCentered(1, pet.def().name);
        snprintf(line, sizeof(line), "%uY %uG", pet.ageYears(), pet.weight());
        lcd.textCentered(9, line);
        break;
    case 1:
        lcd.textCentered(1, "HUNGRY");
        drawHearts(lcd, pet.hunger(), HEARTS_MAX);
        break;
    case 2:
        lcd.textCentered(1, "HAPPY");
        drawHearts(lcd, pet.happy(), HEARTS_MAX);
        break;
    case 3:
        lcd.textCentered(1, "DISC");
        for (uint8_t i = 0; i < DISCIPLINE_MAX; i++)
        {
            lcd.fill(2 + i * 7, 9, 6, 5);
            if (i >= pet.discipline())
            {
                lcd.fill(3 + i * 7, 10, 4, 3, false);
            }
        }
        break;
    default:
        snprintf(line, sizeof(line), "GEN %u", pet.generation());
        lcd.textCentered(1, line);
        snprintf(line, sizeof(line), "BEST %uY", pet.bestYears());
        lcd.textCentered(9, line);
        break;
    }
}

void HomeScreen::drawClock()
{
    char line[12];
    uint32_t seconds = pet.clockSeconds();
    snprintf(line, sizeof(line), "%02u:%02u", (unsigned)(seconds / 3600), (unsigned)(seconds / 60 % 60));
    lcd.textCentered(5, line);
}

void HomeScreen::drawScene()
{
    Pose pose;
    pose.second = sceneStep % 2 == 1;
    const Sprite &art = FORM_ART[shownForm];
    int16_t center = (LcdMatrix::COLS - art.w) / 2;
    int16_t left = 1;
    switch (scene)
    {
    case Scene::EAT_MEAL:
    case Scene::EAT_SNACK:
    {
        const Sprite &food = scene == Scene::EAT_MEAL ? ART_MEAL : ART_SNACK;
        facingLeft = false;
        pose.mouthOpen = sceneStep % 2 == 0;
        lcd.spriteOn(art, left, LcdMatrix::ROWS, false, pose);
        int16_t foodX = LcdMatrix::COLS - food.w - 1;
        int16_t foodY = LcdMatrix::ROWS - food.h;
        lcd.sprite(food, foodX, foodY);
        uint8_t bites = sceneStep / 2;
        for (int16_t y = 0; bites > 0 && y < food.h; y++)
        {
            lcd.fill(foodX, foodY + y, bites * 3 + (y + bites) % 2, 1, false);
        }
        break;
    }
    case Scene::REFUSE:
        lcd.spriteOn(art, center, LcdMatrix::ROWS, sceneStep % 2 == 1, pose);
        break;
    case Scene::CHEER:
        pose.cheer = sceneStep % 2 == 0;
        pose.mouthOpen = true;
        pose.eyesShut = sceneStep % 2 == 1;
        lcd.spriteOn(art, center, LcdMatrix::ROWS - (sceneStep % 2 == 0 ? 1 : 0), false, pose);
        lcd.sprite(ART_SPARK, 2, sceneStep % 2 == 0 ? 1 : 3);
        lcd.sprite(ART_SPARK, LcdMatrix::COLS - 7, sceneStep % 2 == 0 ? 3 : 1);
        break;
    case Scene::FLUSH:
    {
        int16_t wave = LcdMatrix::COLS - 2 - sceneStep * 3;
        drawPetAt(petX < wave - art.w ? petX : left, pose);
        for (uint8_t i = 0; i < flushedPoops; i++)
        {
            drawPoop(i);
        }
        lcd.fill(wave > 0 ? wave : 0, 0, LcdMatrix::COLS, LcdMatrix::ROWS, false);
        for (int16_t y = 0; y < LcdMatrix::ROWS; y++)
        {
            lcd.fill(wave + (y / 2 + sceneStep) % 3, y, 4, 1);
        }
        break;
    }
    case Scene::INJECT:
    {
        pose.eyesShut = true;
        lcd.spriteOn(art, left, LcdMatrix::ROWS, false, pose);
        int16_t near = LcdMatrix::COLS - ART_SYRINGE.w - sceneStep;
        lcd.sprite(ART_SYRINGE, near < art.w ? art.w : near, 5);
        break;
    }
    case Scene::SCOLD:
        lcd.spriteOn(art, left, LcdMatrix::ROWS, false, pose);
        if (sceneStep % 2 == 0)
        {
            lcd.sprite(ART_ANGRY, LcdMatrix::COLS - 9, 1);
            lcd.text(LcdMatrix::COLS - 4, 9, "!");
        }
        break;
    case Scene::HATCH:
    {
        const Sprite &egg = FORM_ART[FORM_EGG];
        if (sceneStep < 5)
        {
            lcd.spriteOn(egg, (LcdMatrix::COLS - egg.w) / 2 + (sceneStep % 2 == 0 ? -1 : 1), LcdMatrix::ROWS, false, pose);
        }
        else
        {
            const Sprite &baby = FORM_ART[pet.form()];
            lcd.spriteOn(baby, (LcdMatrix::COLS - baby.w) / 2, LcdMatrix::ROWS, false, pose);
            lcd.sprite(ART_SPARK, 4, 3);
            lcd.sprite(ART_SPARK, LcdMatrix::COLS - 9, 3);
        }
        break;
    }
    case Scene::EVOLVE:
    {
        const Sprite &next = FORM_ART[pet.form()];
        if (sceneStep < 6)
        {
            const Sprite &show = sceneStep % 2 == 0 ? art : next;
            lcd.spriteOn(show, (LcdMatrix::COLS - show.w) / 2, LcdMatrix::ROWS, false, pose);
        }
        else
        {
            pose.cheer = true;
            pose.mouthOpen = true;
            lcd.spriteOn(next, (LcdMatrix::COLS - next.w) / 2, LcdMatrix::ROWS, false, pose);
            lcd.sprite(ART_SPARK, 1, 2);
            lcd.sprite(ART_SPARK, LcdMatrix::COLS - 6, 2);
        }
        break;
    }
    case Scene::DEATH:
    {
        pose.eyesShut = true;
        int16_t rise = sceneStep * 2;
        if (rise < LcdMatrix::ROWS + 3)
        {
            lcd.sprite(art, center, LcdMatrix::ROWS - art.h - rise, false, pose);
            lcd.sprite(ART_HALO, center + (art.w - ART_HALO.w) / 2, LcdMatrix::ROWS - art.h - rise - 3);
            int16_t wingY = LcdMatrix::ROWS - art.h / 2 - 3 - rise;
            bool up = sceneStep % 2 == 0;
            lcd.sprite(ART_WING, center - ART_WING.w, wingY - (up ? 1 : 0));
            lcd.sprite(ART_WING, center + art.w, wingY - (up ? 1 : 0), true);
        }
        break;
    }
    default:
        break;
    }
}

void HomeScreen::drawGrave()
{
    char line[12];
    if (graveStep == 0)
    {
        lcd.sprite(ART_GRAVE, (LcdMatrix::COLS - ART_GRAVE.w) / 2, 1);
        return;
    }
    snprintf(line, sizeof(line), "GEN %u", pet.generation());
    lcd.textCentered(1, line);
    snprintf(line, sizeof(line), "AGE %uY", pet.ageYears());
    lcd.textCentered(9, line);
}

void HomeScreen::act(Action action, Scene success)
{
    Refusal refusal = pet.act(action);
    changed();
    if (refusal == Refusal::NONE)
    {
        if (success == Scene::EAT_MEAL || success == Scene::EAT_SNACK)
        {
            sound.playEat();
        }
        else
        {
            sound.playCare();
        }
        if (success == Scene::NONE)
        {
            mode = Mode::PET;
        }
        else
        {
            startScene(success);
        }
    }
    else if (refusal == Refusal::NOT_ALIVE)
    {
        sound.playRefuse();
    }
    else if (refusal == Refusal::ASLEEP)
    {
        sound.playRefuse();
        mode = Mode::PET;
    }
    else
    {
        sound.playRefuse();
        startScene(Scene::REFUSE);
    }
    draw();
}

void HomeScreen::confirmIcon()
{
    menuRow = 0;
    switch (selected)
    {
    case ICON_FOOD:
        if (pet.check(Action::SNACK) == Refusal::NOT_ALIVE || pet.isSleeping())
        {
            sound.playRefuse();
            return;
        }
        mode = Mode::FOOD_MENU;
        sound.playMenu();
        break;
    case ICON_LIGHT:
        if (pet.check(Action::LIGHT_ON) == Refusal::NOT_ALIVE)
        {
            sound.playRefuse();
            return;
        }
        menuRow = pet.lightsOn() ? 0 : 1;
        mode = Mode::LIGHT_MENU;
        sound.playMenu();
        break;
    case ICON_GAME:
    {
        Refusal refusal = pet.check(Action::PLAY);
        if (refusal == Refusal::NONE)
        {
            sound.playMenu();
            screenManager.openModal(&gameScreen);
        }
        else
        {
            act(Action::PLAY, Scene::NONE);
        }
        return;
    }
    case ICON_MEDICINE:
        act(Action::MEDICINE, Scene::INJECT);
        return;
    case ICON_BATH:
        flushedPoops = pet.poops();
        act(Action::CLEAN, Scene::FLUSH);
        return;
    case ICON_METER:
        meterPage = 0;
        mode = Mode::METER;
        sound.playMenu();
        break;
    case ICON_DISCIPLINE:
        act(Action::DISCIPLINE, Scene::SCOLD);
        return;
    default:
        return;
    }
    draw();
}

void HomeScreen::moveSelection(int8_t delta)
{
    const int8_t count = ICON_ATTENTION;
    if (selected == Panel::NONE)
    {
        selected = delta > 0 ? 0 : count - 1;
    }
    else
    {
        selected = (int8_t)((selected + count + delta) % count);
    }
    sound.playMenu();
}

void HomeScreen::onConfirmPress()
{
    input();
    switch (mode)
    {
    case Mode::PET:
        if (selected != Panel::NONE)
        {
            confirmIcon();
        }
        break;
    case Mode::FOOD_MENU:
        if (menuRow == 0)
        {
            act(Action::MEAL, Scene::EAT_MEAL);
        }
        else
        {
            act(Action::SNACK, Scene::EAT_SNACK);
        }
        break;
    case Mode::LIGHT_MENU:
        act(menuRow == 0 ? Action::LIGHT_ON : Action::LIGHT_OFF, Scene::NONE);
        break;
    case Mode::METER:
        meterPage = (meterPage + 1) % METER_PAGES;
        sound.playMenu();
        draw();
        break;
    case Mode::GRAVE:
        sound.playMenu();
        if (++graveStep > 1)
        {
            pet.newEgg();
            changed();
            shownForm = FORM_EGG;
            graveStep = 0;
            mode = Mode::PET;
        }
        draw();
        break;
    default:
        break;
    }
}

void HomeScreen::back()
{
    mode = Mode::PET;
    sound.playMenu();
    draw();
}

void HomeScreen::onUpPress()
{
    input();
    if (mode == Mode::PET)
    {
        selected = Panel::NONE;
    }
    else if (mode == Mode::FOOD_MENU || mode == Mode::LIGHT_MENU || mode == Mode::METER || mode == Mode::CLOCK)
    {
        back();
    }
}

void HomeScreen::onDownPress()
{
    input();
    if (mode == Mode::PET)
    {
        mode = Mode::CLOCK;
        clockEndsAt = millis() + CLOCK_MS;
        sound.playMenu();
        draw();
    }
    else if (mode == Mode::CLOCK)
    {
        back();
    }
}

bool HomeScreen::onBackPress()
{
    input();
    if (mode == Mode::PET)
    {
        moveSelection(-1);
    }
    else if (mode == Mode::FOOD_MENU || mode == Mode::LIGHT_MENU)
    {
        menuRow = 1 - menuRow;
        sound.playMenu();
        draw();
    }
    return true;
}

bool HomeScreen::onRightPress()
{
    input();
    if (mode == Mode::PET)
    {
        moveSelection(1);
    }
    else if (mode == Mode::FOOD_MENU || mode == Mode::LIGHT_MENU)
    {
        menuRow = 1 - menuRow;
        sound.playMenu();
        draw();
    }
    return true;
}
