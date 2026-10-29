#include "PlayScreen.h"
#include <math.h>
#include "../art/IdlePose.h"
#include "../gfx/Widgets.h"

// Up, right, down, left: high to low around the pad, so a tune is heard as a shape
static const uint16_t HUM_NOTES[4] = {1319, 1047, 698, 880};
static const uint16_t HUM_NOTE_MS = 380;
static const uint16_t HUM_GAP_MS = 160;
static const uint16_t HUM_MISS_MS = 1400;
static const float PAD_X = 120, PAD_Y = 106, PAD_R = 24;
static const float PAD_AT[4][2] = {{0, -50}, {56, 0}, {0, 50}, {-56, 0}};
static const Icon PAD_ICONS[4] = {Icon::ARROW_UP, Icon::ARROW_RIGHT, Icon::ARROW_DOWN, Icon::ARROW_LEFT};
static const uint8_t PAD_COLORS[4] = {GOLD, BLUSH, WATER, LEAF};

static const unsigned long CATCH_MS = 30000;
static const float FIELD_X = 8, FIELD_Y = 46, FIELD_W = 224, FIELD_H = 246;
static const float CATCH_GROUND = FIELD_Y + FIELD_H - 10;
static const float CATCH_SCALE = 0.7f;
static const Icon TREAT_ICONS[3] = {Icon::CANDY, Icon::STAR, Icon::HEART};
static const uint8_t TREAT_COLORS[3] = {BLUSH, GOLD, ACCENT};

static const char *const GAME_NAMES[] = {"Hum", "Catch"};
static const char *const GAME_NOTES[] = {"Sing its little tune back", "Catch the falling treats"};

void PlayScreen::onEnter(TFT_eSPI &tft)
{
    state = State::MENU;
    lastUpdate = millis();
    Screen::onEnter(tft);
}

void PlayScreen::onUpPress()
{
    if (state == State::MENU)
    {
        selected = selected > 0 ? selected - 1 : GAME_COUNT - 1;
        game.sound.playMenu();
    }
    else if (state == State::HUM_SING)
    {
        sing(0, millis());
    }
    forceFrame();
}

void PlayScreen::onDownPress()
{
    if (state == State::MENU)
    {
        selected = (selected + 1) % GAME_COUNT;
        game.sound.playMenu();
    }
    else if (state == State::HUM_SING)
    {
        sing(2, millis());
    }
    forceFrame();
}

bool PlayScreen::onBackPress()
{
    if (state == State::MENU)
    {
        return false;
    }
    if (state == State::HUM_SING)
    {
        sing(3, millis());
    }
    else if (state == State::CATCH)
    {
        moveCatcher(-1);
    }
    forceFrame();
    return true;
}

bool PlayScreen::onRightPress()
{
    if (state == State::MENU)
    {
        return false;
    }
    if (state == State::HUM_SING)
    {
        sing(1, millis());
    }
    else if (state == State::CATCH)
    {
        moveCatcher(1);
    }
    forceFrame();
    return true;
}

bool PlayScreen::onRepeat(Key key)
{
    if (state == State::CATCH && (key == Key::LEFT || key == Key::RIGHT))
    {
        moveCatcher(key == Key::LEFT ? -1 : 1);
        return true;
    }
    if (state == State::MENU && (key == Key::UP || key == Key::DOWN))
    {
        key == Key::UP ? onUpPress() : onDownPress();
        return true;
    }
    return false;
}

void PlayScreen::onConfirmPress()
{
    unsigned long now = millis();
    if (state == State::MENU)
    {
        if (game.pet.checkPlay() != Refusal::NONE)
        {
            game.sound.playError();
            return;
        }
        start(now);
    }
    else if (state == State::RESULT)
    {
        state = State::MENU;
        game.sound.playMenu();
    }
    forceFrame();
}

void PlayScreen::start(unsigned long now)
{
    score = 0;
    newBest = false;
    stateAt = now;
    game.sound.playStart();
    if (selected == HUM)
    {
        tuneLength = 0;
        startRound(now + 600);
        return;
    }
    state = State::CATCH;
    catcherX = catcherTo = 120;
    for (auto &treat : treats)
    {
        treat.falling = false;
    }
    fallSpeed = 70;
    nextSpawn = now + 700;
}

void PlayScreen::startRound(unsigned long now)
{
    if (tuneLength < HUM_MAX)
    {
        tune[tuneLength++] = (uint8_t)random(4);
    }
    position = 0;
    state = State::HUM_LISTEN;
    nextAt = now + 500;
    lit = -1;
}

void PlayScreen::sing(uint8_t arrow, unsigned long now)
{
    lit = arrow;
    litAt = now;
    litUntil = now + 240;
    if (arrow != tune[position])
    {
        game.sound.playMiss();
        state = State::HUM_MISS;
        nextAt = now + HUM_MISS_MS;
        return;
    }
    game.sound.playNote(HUM_NOTES[arrow], 160);
    if (++position == tuneLength)
    {
        score = tuneLength;
        startRound(now + 450);
    }
}

void PlayScreen::moveCatcher(int8_t dir)
{
    float half = FORM_RIG[game.pet.getForm()].w * CATCH_SCALE * 0.5f;
    catcherTo = fmaxf(FIELD_X + half + 4, fminf(FIELD_X + FIELD_W - half - 4, catcherTo + dir * 18));
}

void PlayScreen::updateCatch(unsigned long now, float dt)
{
    if (now - stateAt >= CATCH_MS)
    {
        finish(now);
        return;
    }
    catcherX += (catcherTo - catcherX) * fminf(1, dt * 14);
    squash.step(dt);
    if ((long)(now - nextSpawn) >= 0)
    {
        for (auto &treat : treats)
        {
            if (!treat.falling)
            {
                treat = {FIELD_X + 14 + random((long)FIELD_W - 28), FIELD_Y + 8, fallSpeed * (0.8f + random(40) / 100.0f),
                         (uint8_t)random(3), true};
                break;
            }
        }
        nextSpawn = now + 550 + random(650);
    }
    const FormRig &rig = FORM_RIG[game.pet.getForm()];
    float top = CATCH_GROUND - rig.h * CATCH_SCALE, half = rig.w * CATCH_SCALE * 0.5f + 6;
    for (auto &treat : treats)
    {
        if (!treat.falling)
        {
            continue;
        }
        treat.y += treat.vy * dt;
        if (treat.y >= top && treat.y < top + 14 && fabsf(treat.x - catcherX) < half)
        {
            treat.falling = false;
            score++;
            caughtAt = now;
            squash.kick(-5);
            game.sound.playNote(1568 + score * 20, 40);
            if (score % 4 == 0 && fallSpeed < 170)
            {
                fallSpeed += 12;
            }
        }
        else if (treat.y > CATCH_GROUND)
        {
            treat.falling = false;
        }
    }
}

void PlayScreen::finish(unsigned long now)
{
    Album &album = game.album;
    if (selected == HUM)
    {
        rewardJoy = PLAY_MIN_JOY + score * 6;
        rewardFood = 0;
        newBest = score > album.humBest;
        album.humBest = newBest ? score : album.humBest;
    }
    else
    {
        rewardJoy = PLAY_MIN_JOY + score * 2;
        rewardFood = score * 2 > 30 ? 30 : score * 2;
        newBest = score > album.catchBest;
        album.catchBest = newBest ? score : album.catchBest;
    }
    if (rewardJoy > PLAY_MAX_JOY)
    {
        rewardJoy = PLAY_MAX_JOY;
    }
    game.pet.playReward(rewardJoy, rewardFood);
    game.saver.soon(now);
    game.sound.playWin();
    state = State::RESULT;
    stateAt = now;
}

void PlayScreen::update(unsigned long now)
{
    float dt = fminf(0.1f, (now - lastUpdate) / 1000.0f);
    lastUpdate = now;
    switch (state)
    {
    case State::HUM_LISTEN:
        if ((long)(now - nextAt) < 0)
        {
            break;
        }
        if (position == tuneLength)
        {
            position = 0;
            state = State::HUM_SING;
            lit = -1;
            break;
        }
        lit = tune[position];
        litAt = now;
        litUntil = now + HUM_NOTE_MS;
        game.sound.playNote(HUM_NOTES[lit], HUM_NOTE_MS);
        position++;
        nextAt = now + HUM_NOTE_MS + HUM_GAP_MS;
        break;
    case State::HUM_MISS:
        if ((long)(now - nextAt) >= 0)
        {
            finish(now);
        }
        break;
    case State::CATCH:
        updateCatch(now, dt);
        break;
    default:
        break;
    }
}

void PlayScreen::drawMenu(Canvas &c) const
{
    unsigned long now = frameTime();
    const uint8_t bests[GAME_COUNT] = {game.album.humBest, game.album.catchBest};
    static const Icon ICONS[GAME_COUNT] = {Icon::NOTE, Icon::CANDY};
    for (uint8_t i = 0; i < GAME_COUNT; i++)
    {
        float y = 38 + i * 74;
        bool on = i == selected;
        if (!c.reaches(y - 4, y + 70))
        {
            continue;
        }
        if (on)
        {
            c.fillRoundRect(8, y - 2, 224, 68, 18, Theme::rgb(ACCENT));
        }
        Widgets::card(c, 10, y, 220, 64, 16);
        float bob = on ? sinf(now / 260.0f) * 2 : 0;
        c.fillCircle(42, y + 32, 20, Theme::rgb(on ? ACCENT : WALL_TOP));
        drawIcon(c, ICONS[i], 42, y + 32 + bob, 26, Theme::rgb(on ? ACCENT_INK : INK_SOFT), Theme::rgb(on ? ACCENT : WALL_TOP));
        c.text(72, y + 12, GAME_NAMES[i], FONT_TITLE, Theme::rgb(INK));
        c.text(72, y + 38, GAME_NOTES[i], FONT_SMALL, Theme::rgb(INK_SOFT));
        char buf[16];
        snprintf(buf, sizeof(buf), "Best %u", (unsigned)bests[i]);
        int16_t bw = c.text(218, y + 14, buf, FONT_SMALL, Theme::rgb(INK_SOFT), Canvas::RIGHT);
        drawIcon(c, Icon::STAR, 218 - bw - 9, y + 21, 12, Theme::rgb(GOLD), Theme::rgb(CARD));
    }
    Refusal refusal = game.pet.checkPlay();
    const char *why = refusal == Refusal::EGG      ? "Eggs don't play yet"
                      : refusal == Refusal::ASLEEP ? "Shh, it's asleep. Later!"
                      : refusal == Refusal::TIRED  ? "Too sleepy to play"
                                                   : "Playing brings joy, and tires a little";
    c.text(120, 190, why, FONT_SMALL, Theme::rgb(refusal == Refusal::NONE ? INK_SOFT : ALERT), Canvas::CENTER);
    c.fillEllipse(120, 294, 40, 6, Theme::rgb(RUG), 200);
    Pose p = idlePose(game.pet, now, 120, 292, 0.75f);
    if (refusal == Refusal::NONE)
    {
        p.eye = Eye::HAPPY;
        p.lift += fabsf(sinf(now / 300.0f)) * 5;
    }
    drawPet(c, game.pet.getForm(), p);
    Widgets::hint(c, 308, "red button plays");
}

void PlayScreen::drawHum(Canvas &c) const
{
    unsigned long now = frameTime();
    bool showLit = lit >= 0 && (long)(now - litUntil) < 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        float cx = PAD_X + PAD_AT[i][0], cy = PAD_Y + PAD_AT[i][1];
        bool on = showLit && lit == i;
        float since = (now - litAt) / 1000.0f;
        float pop = on ? 1 + 0.18f * expf(-since * 8) : 1;
        uint16_t color = Theme::rgb(PAD_COLORS[i]);
        c.fillCircle(cx, cy + 3, PAD_R * pop, Theme::rgb(SHADOW), 90);
        c.fillCircle(cx, cy, PAD_R * pop, on ? color : Canvas::mix(color, Theme::rgb(CARD), 150));
        drawIcon(c, PAD_ICONS[i], cx, cy, 26 * pop, Theme::rgb(on ? SHINE : INK_SOFT), color);
        if (on)
        {
            c.ring(cx, cy, PAD_R * pop + 6 + since * 30, 2, color, (uint8_t)fmaxf(0, 200 - since * 900));
        }
    }
    FormId form = game.pet.getForm();
    bool singing = state == State::HUM_LISTEN && showLit;
    Pose p = idlePose(game.pet, now, 120, 296, 0.62f);
    p.eye = state == State::HUM_MISS ? Eye::SAD : singing ? Eye::CLOSED : Eye::HAPPY;
    p.mouth = state == State::HUM_MISS ? Mouth::FROWN : singing ? Mouth::OPEN : Mouth::W;
    if (singing)
    {
        p.squashY += 0.06f;
        p.squashX -= 0.04f;
    }
    c.fillEllipse(120, 298, 36, 5, Theme::rgb(RUG), 200);
    drawPet(c, form, p);
    if (singing)
    {
        float rise = (now - litAt) * 0.05f;
        drawIcon(c, Icon::NOTE, 156, 250 - rise, 16, Theme::rgb(PAD_COLORS[lit]), Theme::rgb(CARD));
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "Round %u", (unsigned)tuneLength);
    c.text(14, 34, buf, FONT_SMALL, Theme::rgb(INK_SOFT));
    const char *prompt = state == State::HUM_LISTEN ? "Listen..."
                         : state == State::HUM_SING ? "Your turn! Sing it back"
                                                    : "Oops, a wrong note";
    c.text(120, 180, prompt, FONT_BODY, Theme::rgb(state == State::HUM_MISS ? ALERT : INK), Canvas::CENTER);
    if (state == State::HUM_SING)
    {
        for (uint8_t i = 0; i < tuneLength && i < 16; i++)
        {
            float dx = 120 + (i - (tuneLength - 1) / 2.0f) * 9;
            c.fillCircle(dx, 202, 3, Theme::rgb(i < position ? ACCENT : CARD_EDGE));
        }
    }
}

void PlayScreen::drawCatch(Canvas &c) const
{
    unsigned long now = frameTime();
    c.fillRoundRectV(FIELD_X, FIELD_Y, FIELD_W, FIELD_H, 16, Theme::rgb(SKY_DAY), Theme::rgb(WALL_TOP));
    c.fillRoundRect(FIELD_X, CATCH_GROUND - 2, FIELD_W, FIELD_Y + FIELD_H - CATCH_GROUND + 2, 8, Theme::rgb(FLOOR));
    for (const auto &treat : treats)
    {
        if (treat.falling)
        {
            float wob = sinf(now / 150.0f + treat.x) * 3;
            drawIcon(c, TREAT_ICONS[treat.kind], treat.x + wob, treat.y, 18, Theme::rgb(TREAT_COLORS[treat.kind]),
                     Theme::rgb(SHINE));
        }
    }
    Pose p = idlePose(game.pet, now, catcherX, CATCH_GROUND, CATCH_SCALE);
    p.squashY = 1 + squash.value;
    p.squashX = 1 - squash.value * 0.7f;
    p.eye = now - caughtAt < 300 ? Eye::HAPPY : Eye::OPEN;
    p.mouth = Mouth::OPEN;
    p.lookY = -1;
    p.blush = 0.8f;
    p.lean = (catcherTo - catcherX) * 0.3f;
    drawPet(c, game.pet.getForm(), p);
    unsigned long left = CATCH_MS - (now - stateAt);
    char buf[24];
    snprintf(buf, sizeof(buf), "%u", (unsigned)score);
    drawIcon(c, Icon::CANDY, 22, 38, 16, Theme::rgb(BLUSH), Theme::rgb(SHINE));
    c.text(34, 30, buf, FONT_BODY, Theme::rgb(INK));
    snprintf(buf, sizeof(buf), "%lus", (left + 999) / 1000);
    c.text(226, 30, buf, FONT_BODY, Theme::rgb(INK), Canvas::RIGHT);
    Widgets::hint(c, 304, "left/right to walk");
}

void PlayScreen::drawResult(Canvas &c) const
{
    unsigned long now = frameTime();
    char buf[40];
    float u = fminf(1, (now - stateAt) / 300.0f);
    c.text(120, 44 - (1 - u) * 12, newBest ? "New best!" : "Well done!", FONT_TITLE, Theme::rgb(newBest ? ACCENT : INK),
           Canvas::CENTER, (uint8_t)(u * 255));
    if (selected == HUM)
    {
        snprintf(buf, sizeof(buf), "You sang %u notes together", (unsigned)score);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Caught %u treats!", (unsigned)score);
    }
    c.text(120, 76, buf, FONT_BODY, Theme::rgb(INK_SOFT), Canvas::CENTER);
    Widgets::card(c, 50, 100, 140, rewardFood > 0 ? 64 : 40, 14);
    drawIcon(c, Icon::STAR, 76, 120, 18, Theme::rgb(GOLD), Theme::rgb(CARD));
    snprintf(buf, sizeof(buf), "+%u joy", (unsigned)rewardJoy);
    c.text(92, 112, buf, FONT_BODY, Theme::rgb(INK));
    if (rewardFood > 0)
    {
        drawIcon(c, Icon::BOWL, 76, 145, 18, Theme::rgb(BLUSH), Theme::rgb(GOLD));
        snprintf(buf, sizeof(buf), "+%u food", (unsigned)rewardFood);
        c.text(92, 137, buf, FONT_BODY, Theme::rgb(INK));
    }
    Pose p = idlePose(game.pet, now, 120, 280, 1.0f);
    p.eye = Eye::HAPPY;
    p.mouth = Mouth::W;
    p.blush = 1;
    float hop = fabsf(sinf(now / 260.0f));
    p.lift = hop * 14;
    p.squashY = 1 + (hop - 0.5f) * 0.12f;
    p.squashX = 2 - p.squashY;
    c.fillEllipse(120, 282, 50, 7, Theme::rgb(RUG), 200);
    drawPet(c, game.pet.getForm(), p);
    for (int i = 0; i < 3; i++)
    {
        float t = fmodf(now / 1400.0f + i / 3.0f, 1.0f);
        drawIcon(c, Icon::HEART, 80 + i * 40, 230 - t * 50, 12, Theme::rgb(BLUSH), Theme::rgb(SHINE),
                 (uint8_t)(255 * (1 - t)));
    }
    Widgets::hint(c, 308, "red button: done");
}

void PlayScreen::drawScene(Canvas &c) const
{
    switch (state)
    {
    case State::MENU:
        drawMenu(c);
        break;
    case State::CATCH:
        drawCatch(c);
        break;
    case State::RESULT:
        drawResult(c);
        break;
    default:
        drawHum(c);
        break;
    }
}
