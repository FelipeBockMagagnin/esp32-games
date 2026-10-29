#include "PlayScreen.h"
#include <esp_random.h>

static const uint16_t STEP_THIRDS = 50; // 1/60 s in thirds of a millisecond
static const uint8_t MAX_STEPS_PER_FRAME = 4;
static const unsigned long MAX_CATCH_UP_MS = 100;
static const unsigned long DYING_MS = 900;
static const uint8_t KILL_SHAKE_STEPS = 10;
static const uint8_t DEATH_SHAKE_STEPS = 30;
static const uint8_t BURST_PARTICLES = 12;
static const uint8_t MAX_COINS = 8;

static const int16_t CENTER_X = 120;
static const int16_t HUD_Y = 8;
static const int16_t FOOTER_TOP_Y = ARENA_PX + PlayScreen::ARENA_TOP + 8;
static const int16_t FOOTER_BOTTOM_Y = FOOTER_TOP_Y + 14;

static uint16_t bands[2][ARENA_PX * ArenaRenderer::BAND_H];

PlayScreen::PlayScreen(Round &round, const Profile &profile, SoundManager &sound, ScreenManager &screenManager,
                       Screen &shop, Screen &settings, void (*died)())
    : leftText(6, HUD_Y + 4, "", Theme::TEXT_2, 2, TL_DATUM),
      scoreText(CENTER_X, HUD_Y, "", Theme::TEXT, 3, TC_DATUM),
      coinText(234, HUD_Y + 4, "", Theme::GOLD, 2, TR_DATUM),
      footerTop(CENTER_X, FOOTER_TOP_Y, "", Theme::TEXT_2, 1, TC_DATUM),
      footerBottom(CENTER_X, FOOTER_BOTTOM_Y, "", Theme::TEXT_DIM, 1, TC_DATUM),
      round(round),
      profile(profile),
      sound(sound),
      screenManager(screenManager),
      shop(shop),
      settings(settings),
      died(died)
{
    addElement(&leftText);
    addElement(&scoreText);
    addElement(&coinText);
    addElement(&footerTop);
    addElement(&footerBottom);
    effects.reset();
}

void PlayScreen::toTitle()
{
    mode = Mode::TITLE;
    // A fresh field for the hammer to stand in; it only steps once a round starts
    round.start(esp_random());
    effects.reset();
    frameDue = true;
    forceRefresh();
}

void PlayScreen::newRound()
{
    round.start(esp_random());
    effects.reset();
    nextBackground();
    mode = Mode::PLAY;
    sound.playStart();
    frameDue = true;
    forceRefresh();
}

void PlayScreen::continueRound()
{
    round.revive();
    effects.reset();
    mode = Mode::PLAY;
    sound.playStart();
    frameDue = true;
    forceRefresh();
}

void PlayScreen::styleChanged()
{
    backgroundIndex = 0;
    frameDue = true;
}

void PlayScreen::nextBackground()
{
    uint8_t count = STYLE_ART[profile.style].backgroundCount;
    if (count < 2)
    {
        backgroundIndex = 0;
        return;
    }
    // Never the same one twice running, as ChangeBackgroundColor took care to do
    backgroundIndex = (uint8_t)((backgroundIndex + 1 + rng.below(count - 1)) % count);
}

void PlayScreen::onEnter(TFT_eSPI &tft)
{
    lastStepAt = millis();
    stepCredit = 0;
    frameDue = true;
    refreshText();
    Screen::onEnter(tft);
}

void PlayScreen::turn(int8_t direction)
{
    if (mode == Mode::DYING)
    {
        return;
    }
    round.turn(direction);
    if (mode == Mode::PLAY)
    {
        sound.playTurn();
        playEvents();
    }
    frameDue = true;
}

void PlayScreen::playEvents()
{
    uint8_t events = round.takeEvents();
    const StyleArt &style = STYLE_ART[profile.style];
    static const int8_t DX[SIDE_COUNT] = {0, 1, 0, -1};
    static const int8_t DY[SIDE_COUNT] = {-1, 0, 1, 0};
    for (uint8_t i = 0; i < round.getMarkCount(); i++)
    {
        const Round::Mark &m = round.getMark(i);
        float x = ARENA_PX / 2 + DX[m.side] * m.distance * PX_PER_UNIT;
        float y = ARENA_PX / 2 + DY[m.side] * m.distance * PX_PER_UNIT;
        const Art &art = *style.enemies[m.look % style.enemyCount];
        effects.burst(x, y, art.accents[0], art.accents[1], BURST_PARTICLES);
        if (!(events & EVENT_DEAD))
        {
            uint32_t coins = round.getMultiplier();
            effects.coins(x, y, (uint8_t)(coins < MAX_COINS ? coins : MAX_COINS));
        }
    }
    round.clearMarks();

    if (events & EVENT_DEAD)
    {
        const Art &player = *style.player;
        effects.burst(ARENA_PX / 2, ARENA_PX / 2, player.accents[0], player.accents[1], 3 * BURST_PARTICLES);
        effects.shake(DEATH_SHAKE_STEPS);
        sound.playDeath();
        mode = Mode::DYING;
        dyingSince = millis();
        deathReported = false;
        forceRefresh();
    }
    else if (events & EVENT_KILL)
    {
        effects.shake(KILL_SHAKE_STEPS);
        nextBackground();
        sound.playKill();
        forceRefresh();
    }
}

void PlayScreen::update(unsigned long now)
{
    unsigned long elapsed = now - lastStepAt;
    lastStepAt = now;
    stepCredit += (elapsed < MAX_CATCH_UP_MS ? elapsed : MAX_CATCH_UP_MS) * 3;
    uint8_t steps = 0;
    uint8_t art = STYLE_ART[profile.style].player->frames;
    while (stepCredit >= STEP_THIRDS && steps < MAX_STEPS_PER_FRAME)
    {
        stepCredit -= STEP_THIRDS;
        steps++;
        tick++;
        if (mode == Mode::PLAY)
        {
            round.step();
            playEvents();
        }
        if (effects.isBusy())
        {
            effects.step();
            frameDue = true;
        }
    }
    if (steps == MAX_STEPS_PER_FRAME)
    {
        stepCredit = 0;
    }
    if (steps > 0 && (mode == Mode::PLAY || art > 1))
    {
        frameDue = true;
    }

    if (mode == Mode::DYING && !deathReported && now - dyingSince >= DYING_MS && !effects.isBusy())
    {
        // Stays DYING under the game-over modal, so the frozen field shows behind it
        deathReported = true;
        died();
        return;
    }

    if (refreshDue(now))
    {
        refreshText();
    }
}

void PlayScreen::refreshText()
{
    char buf[24];
    if (mode == Mode::TITLE)
    {
        snprintf(buf, sizeof(buf), "Best %lu", (unsigned long)profile.best);
        leftText.setText(buf);
        leftText.setSize(1);
        scoreText.setText("JackHammer");
        scoreText.setSize(2);
        footerTop.setText("Confirm: start");
        footerBottom.setText("Up: shop - Down: settings");
    }
    else
    {
        snprintf(buf, sizeof(buf), "x%lu", (unsigned long)round.getMultiplier());
        leftText.setText(buf);
        leftText.setSize(2);
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)round.getPoints());
        scoreText.setText(buf);
        scoreText.setSize(3);
        snprintf(buf, sizeof(buf), "Best %lu", (unsigned long)(round.getPoints() > profile.best ? round.getPoints()
                                                                                               : profile.best));
        footerTop.setText(buf);
        footerBottom.setText("Left/Right turn - Confirm turns too");
    }
    // The round's coins show as they come, though they are banked at its end
    uint32_t coins = profile.coins + round.getUnbanked();
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)coins);
    coinText.setText(buf);
}

void PlayScreen::render(TFT_eSPI &tft)
{
    Screen::render(tft);
    if (frameDue)
    {
        pushFrame(tft);
        frameDue = false;
    }
}

void PlayScreen::pushFrame(TFT_eSPI &tft)
{
    const StyleArt &style = STYLE_ART[profile.style];
    ArenaRenderer::Scene scene = {&style, style.backgrounds[backgroundIndex % style.backgroundCount], &round,
                                  &effects, tick, !(mode == Mode::DYING && round.isDead())};
    tft.startWrite();
    for (uint8_t b = 0; b < ArenaRenderer::BAND_COUNT; b++)
    {
        uint16_t *buffer = bands[b & 1];
        renderer.renderBand(scene, b, buffer);
        // Waits for the previous band's transfer, which ran while this one was drawn
        tft.pushImageDMA(0, ARENA_TOP + b * ArenaRenderer::BAND_H, ARENA_PX, ArenaRenderer::BAND_H, buffer);
    }
    tft.dmaWait();
    tft.endWrite();
}

void PlayScreen::onConfirmPress()
{
    if (mode == Mode::TITLE)
    {
        newRound();
    }
    else
    {
        turn(1);
    }
}

void PlayScreen::onUpPress()
{
    if (mode == Mode::TITLE)
    {
        sound.playMenu();
        screenManager.openModal(&shop);
    }
}

void PlayScreen::onDownPress()
{
    if (mode == Mode::TITLE)
    {
        sound.playMenu();
        screenManager.openModal(&settings);
    }
}

bool PlayScreen::onBackPress()
{
    turn(-1);
    return true;
}

bool PlayScreen::onRightPress()
{
    turn(1);
    return true;
}
