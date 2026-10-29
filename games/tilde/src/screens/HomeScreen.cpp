#include "HomeScreen.h"
#include <math.h>
#include "Lines.h"
#include "../art/Room.h"
#include "../gfx/Widgets.h"

static const char *const ITEM_NAMES[] = {"Meal", "Snack", "Play", "Clean", "Pet", "Lamp", "Meds"};
static const char *const ITEM_NOTES[] = {"a full bowl", "a little treat", "a minigame", "sweep the fluff",
                                         "a cuddle", "lights on or off", "for a tummy ache"};
static const Icon ITEM_ICONS[] = {Icon::BOWL, Icon::CANDY, Icon::BALL, Icon::BROOM, Icon::HEART, Icon::BULB, Icon::CROSS};
static const Action ITEM_ACTIONS[] = {Action::MEAL, Action::SNACK, Action::COUNT, Action::CLEAN,
                                      Action::CUDDLE, Action::LIGHT, Action::MEDS};
static const Anim ITEM_ANIMS[] = {Anim::MEAL, Anim::SNACK, Anim::NONE, Anim::CLEAN,
                                  Anim::CUDDLE, Anim::NONE, Anim::MEDS};

static const float NEEDS_Y = 220;
static const float BAR_Y = 274;
static const float BUTTON_R = 13;
static const float LABEL_Y = 300;
static const unsigned long FLAVOR_MS = 18000;
static const unsigned long FLAVOR_SHOWN_MS = 5000;

HomeScreen::HomeScreen(Display &display, Game &game, Screen &hatchScreen, void (*openPlay)())
    : Screen(display), game(game), hatchScreen(hatchScreen), openPlay(openPlay)
{
}

void HomeScreen::restart(unsigned long now)
{
    seenForm = game.pet.getForm();
    hatchPending = false;
    selected = 0;
    view.reset(now, game.pet);
    messageUntil = 0;
    nextFlavor = now + 3000;
}

void HomeScreen::onEnter(TFT_eSPI &tft)
{
    unsigned long now = millis();
    lastUpdate = now;
    if (seenForm != game.pet.getForm() && !hatchPending)
    {
        // Grew while another screen was open: the change still plays here
        update(now);
    }
    else
    {
        view.reset(now, game.pet);
        if (!game.pet.isEgg() && !game.pet.isAsleep())
        {
            view.play(Anim::CHEER, now, game.pet);
        }
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        shownNeeds[i] = i == 3 ? game.pet.getTidy() : game.pet.getNeed((Need)i);
    }
    Screen::onEnter(tft);
}

void HomeScreen::say(const char *line, unsigned long now, unsigned long ms)
{
    snprintf(message, sizeof(message), line, game.pet.getName());
    messageAt = now;
    messageUntil = now + ms;
    nextFlavor = now + ms + FLAVOR_MS;
}

void HomeScreen::update(unsigned long now)
{
    Pet &pet = game.pet;
    float dt = fminf(0.1f, (now - lastUpdate) / 1000.0f);
    lastUpdate = now;
    if (pet.getForm() != seenForm)
    {
        if (seenForm == FORM_EGG)
        {
            view.reset(now, pet);
            view.play(Anim::HATCH, now, pet);
            hatchPending = true;
            messageUntil = now;
            nextFlavor = now + 8000;
        }
        else
        {
            view.play(Anim::EVOLVE, now, pet);
            char line[48];
            snprintf(line, sizeof(line), "%%s grew into a %s!", FORMS[pet.getForm()].name);
            say(line, now, 6000);
        }
        seenForm = pet.getForm();
        selected = 0;
    }
    if (hatchPending && view.isPlaying(now) && view.elapsed(now) >= 1900 && (long)(now - messageUntil) >= 0)
    {
        say("Hello! It hatched!", now, 900);
    }
    if (hatchPending && !view.isPlaying(now))
    {
        hatchPending = false;
        game.screens.openModal(&hatchScreen);
        return;
    }
    view.update(now, pet);

    if (pet.isEgg())
    {
        if ((long)(now - messageUntil) >= 0)
        {
            snprintf(message, sizeof(message), "%s", "Press the red button to warm it!");
            messageAt = now - 1000;
            messageUntil = now + 1000;
        }
    }
    else if (pet.isAsleep() && !pet.isLightOn() && pet.getNeed(NEED_REST) >= NEED_MAX &&
             (long)(now - messageUntil) >= 0)
    {
        say("%s is rested. Lamp on to wake it!", now, 5000);
    }
    else if ((long)(now - nextFlavor) >= 0 && !view.isPlaying(now) && !hatchPending)
    {
        say(MOOD_LINES[(uint8_t)pet.mood()][random(LINES_PER_MOOD)], now, FLAVOR_SHOWN_MS);
    }

    float targets[4] = {(float)pet.getNeed(NEED_FOOD), (float)pet.getNeed(NEED_JOY), (float)pet.getNeed(NEED_REST),
                        (float)pet.getTidy()};
    for (uint8_t i = 0; i < 4; i++)
    {
        shownNeeds[i] += (targets[i] - shownNeeds[i]) * fminf(1, dt * 5);
    }
    pop.step(dt);

    snprintf(titleText, sizeof(titleText), "%s", pet.isEgg() ? "An egg" : pet.getName());
}

uint8_t HomeScreen::itemCount() const
{
    return game.pet.isEgg() ? 1 : ITEM_COUNT;
}

bool HomeScreen::available(uint8_t item) const
{
    if (game.pet.isEgg())
    {
        return true;
    }
    if (item == PLAY)
    {
        return game.pet.checkPlay() == Refusal::NONE;
    }
    return game.pet.check(ITEM_ACTIONS[item]) == Refusal::NONE;
}

float HomeScreen::buttonX(uint8_t item) const
{
    uint8_t count = itemCount();
    const float pitch = 32;
    return Canvas::W / 2 + (item - (count - 1) / 2.0f) * pitch;
}

void HomeScreen::select(uint8_t item, unsigned long now)
{
    selected = item;
    pop.kick(6);
    game.sound.playMenu();
    if (!game.pet.isEgg())
    {
        view.lookAt(buttonX(item), BAR_Y, now);
    }
    forceFrame();
}

void HomeScreen::onUpPress()
{
    select(selected > 0 ? selected - 1 : itemCount() - 1, millis());
}

void HomeScreen::onDownPress()
{
    select((selected + 1) % itemCount(), millis());
}

bool HomeScreen::onRepeat(Key key)
{
    if (key == Key::UP || key == Key::DOWN)
    {
        key == Key::UP ? onUpPress() : onDownPress();
        return true;
    }
    // Holding Confirm on the egg keeps warming it
    if (key == Key::CONFIRM && game.pet.isEgg())
    {
        onConfirmPress();
        return true;
    }
    return false;
}

void HomeScreen::onConfirmPress()
{
    unsigned long now = millis();
    // Care waits for care, but a reaction (a cheer, a startle) gives way
    Anim playing = view.isPlaying(now) ? view.playing() : Anim::NONE;
    if (hatchPending || (playing != Anim::NONE && playing != Anim::CHEER && playing != Anim::STARTLE &&
                         playing != Anim::WAKE && playing != Anim::REFUSE))
    {
        return;
    }
    pop.kick(-5);
    doItem(game.pet.isEgg() ? (uint8_t)ITEM_COUNT : selected, now);
    forceFrame();
}

void HomeScreen::doItem(uint8_t item, unsigned long now)
{
    Pet &pet = game.pet;
    if (item == ITEM_COUNT)
    {
        pet.act(Action::WARM);
        game.sound.playMenu();
        say(ACTION_LINES[(uint8_t)Action::WARM], now, 1500);
        return;
    }
    if (item == PLAY)
    {
        Refusal refusal = pet.checkPlay();
        if (refusal != Refusal::NONE)
        {
            game.sound.playError();
            view.play(Anim::REFUSE, now, pet);
            say(REFUSAL_LINES[(uint8_t)refusal], now);
            return;
        }
        game.sound.playMenu();
        openPlay();
        return;
    }
    Action action = ITEM_ACTIONS[item];
    Refusal refusal = pet.check(action);
    if (refusal != Refusal::NONE)
    {
        game.sound.playError();
        if (!pet.isAsleep())
        {
            view.play(Anim::REFUSE, now, pet);
        }
        say(REFUSAL_LINES[(uint8_t)refusal], now);
        return;
    }
    bool wasAsleep = pet.isAsleep();
    pet.act(action);
    game.saver.soon(now);
    if (ITEM_ANIMS[item] != Anim::NONE)
    {
        view.play(ITEM_ANIMS[item], now, pet);
    }
    switch (action)
    {
    case Action::MEAL:
    case Action::SNACK:
        game.sound.playEat();
        break;
    case Action::CUDDLE:
        game.sound.playVoice(FORM_RIG[pet.getForm()].voice);
        break;
    case Action::CLEAN:
        game.sound.playClean();
        break;
    case Action::MEDS:
        game.sound.playMeds();
        break;
    case Action::LIGHT:
        game.sound.playLamp(pet.isLightOn());
        if (pet.isAsleep() && !wasAsleep)
        {
            game.sound.playSleep();
            say("Lamp off. Good night, %s!", now);
        }
        else
        {
            say(pet.isLightOn() ? "The lamp is on." : "The lamp is off.", now, 2500);
        }
        return;
    default:
        break;
    }
    say(ACTION_LINES[(uint8_t)action], now);
}

void HomeScreen::drawNeeds(Canvas &c) const
{
    if (!c.reaches(NEEDS_Y - 2, NEEDS_Y + 30))
    {
        return;
    }
    static const Icon ICONS[4] = {Icon::BOWL, Icon::STAR, Icon::MOON, Icon::SPARKLE};
    static const uint8_t COLORS[4] = {BLUSH, GOLD, WATER, LEAF};
    unsigned long now = frameTime();
    for (uint8_t i = 0; i < 4; i++)
    {
        float x = 8 + i * 57, w = 54;
        Widgets::card(c, x, NEEDS_Y, w, 28, 14);
        bool low = shownNeeds[i] < NEED_LOW && !game.pet.isEgg();
        uint8_t color = low ? ALERT : COLORS[i];
        float pulse = low ? 1 + 0.12f * sinf(now / 160.0f) : 1;
        drawIcon(c, ICONS[i], x + 13, NEEDS_Y + 14, 15 * pulse, Theme::rgb(color), Theme::rgb(CARD));
        Widgets::meter(c, x + 23, NEEDS_Y + 10, w - 29, 8, game.pet.isEgg() ? 1 : shownNeeds[i] / NEED_MAX,
                       game.pet.isEgg() ? INK_FAINT : color);
    }
}

void HomeScreen::drawBar(Canvas &c) const
{
    if (!c.reaches(BAR_Y - 22, LABEL_Y + 20))
    {
        return;
    }
    uint8_t count = itemCount();
    Widgets::card(c, 6, BAR_Y - 20, Canvas::W - 12, 40, 20);
    for (uint8_t i = 0; i < count; i++)
    {
        bool on = i == selected;
        Icon icon = game.pet.isEgg() ? Icon::HEART : ITEM_ICONS[i];
        Widgets::button(c, buttonX(i), BAR_Y, on ? BUTTON_R + 3 : BUTTON_R, icon, on, available(i), on ? pop.value : 1);
    }
    const char *name = game.pet.isEgg() ? "Warm" : ITEM_NAMES[selected];
    const char *note = game.pet.isEgg() ? "keep it cosy" : ITEM_NOTES[selected];
    int16_t nw = Canvas::textWidth(name, FONT_BODY), gap = 6, tw = Canvas::textWidth(note, FONT_SMALL);
    float x0 = Canvas::W / 2.0f - (nw + gap + tw) / 2.0f;
    c.text(x0, LABEL_Y + 2, name, FONT_BODY, Theme::rgb(available(selected) ? INK : INK_FAINT));
    c.text(x0 + nw + gap, LABEL_Y + 4, note, FONT_SMALL, Theme::rgb(INK_SOFT));
}

void HomeScreen::drawBubble(Canvas &c, unsigned long now) const
{
    if (message[0] == '\0' || (long)(now - messageUntil) >= 0)
    {
        return;
    }
    // Pops in, and fades its last moment
    float in = fminf(1, (now - messageAt) / 180.0f);
    float scale = in < 1 ? 0.8f + 0.2f * in + 0.08f * sinf(in * (float)M_PI) : 1;
    long left = (long)(messageUntil - now);
    uint8_t alpha = left < 300 ? (uint8_t)(left * 255 / 300) : 255;
    const Font &font = Canvas::textWidth(message, FONT_BODY) <= 196 ? FONT_BODY : FONT_SMALL;
    float tw = Canvas::textWidth(message, font), w = (tw + 22) * scale, h = 24 * scale;
    float px = view.getX(), cx = fmaxf(Room::X + w / 2 + 4, fminf(Room::X + Room::W - w / 2 - 4, px));
    float y = Room::Y + 8;
    if (!c.reaches(y - 2, y + h + 10))
    {
        return;
    }
    c.setOpacity(alpha);
    uint16_t card = Theme::rgb(CARD);
    c.fillRoundRect(cx - w / 2, y + 2, w, h, h / 2, Theme::rgb(SHADOW), 80);
    c.fillTriangle(px - 6, y + h - 1, px + 6, y + h - 1, px + 2, y + h + 8, card);
    c.fillRoundRect(cx - w / 2, y, w, h, h / 2, card);
    if (scale > 0.85f)
    {
        c.text(cx, y + (h - font.height) / 2, message, font, Theme::rgb(INK), Canvas::CENTER);
    }
    c.setOpacity(255);
}

// The header, the room and what is under it: the room goes out whole, so the pet is never torn
uint8_t HomeScreen::segments(int16_t *edges) const
{
    edges[0] = 0;
    edges[1] = Room::Y;
    edges[2] = Room::Y + Room::H;
    edges[3] = Canvas::H;
    return 3;
}

// The header and the bar under the room change only with what they show; the room moves
// every frame and is always drawn
uint32_t HomeScreen::bandKey(uint8_t segment) const
{
    unsigned long now = frameTime();
    if (segment == 0)
    {
        return headerKey();
    }
    if (segment == 1)
    {
        return 0;
    }
    uint32_t key = keyMix(2166136261u, selected);
    key = keyMix(key, game.pet.isEgg());
    key = keyMix(key, (uint32_t)lroundf(pop.value * 256));
    bool low = false;
    for (uint8_t i = 0; i < 4; i++)
    {
        key = keyMix(key, (uint32_t)lroundf(shownNeeds[i] * 4));
        low = low || (shownNeeds[i] < NEED_LOW && !game.pet.isEgg());
    }
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        key = keyMix(key, available(i));
    }
    // A low need's icon pulses
    return low ? keyMix(key, now / 33) : key;
}

void HomeScreen::drawScene(Canvas &c) const
{
    unsigned long now = frameTime();
    const Pet &pet = game.pet;
    const Room::State &room = view.room();
    Room::drawBack(c, pet, room, now);
    // Kept to the room: the bands under it are not redrawn every frame, so a confetti falling
    // into them would stay there
    c.setClip(Room::X, Room::Y, Room::X + Room::W, Room::Y + Room::H);
    view.draw(c, pet, now);
    Room::drawNight(c, room, now);
    view.drawGlow(c, now);
    c.clearClip();
    drawBubble(c, now);
    drawNeeds(c);
    drawBar(c);
}
