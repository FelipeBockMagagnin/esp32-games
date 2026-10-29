#include "PlayScreen.h"
#include <esp_random.h>

static const uint16_t STEP_THIRDS = 50; // 1/60 s in thirds of a millisecond
static const uint8_t MAX_STEPS_PER_FRAME = 4;
static const unsigned long MAX_CATCH_UP_MS = 100;
static const unsigned long DYING_MS = 900;
static const uint8_t KILL_SHAKE_STEPS = 10;
static const uint8_t DEATH_SHAKE_STEPS = 30;
static const uint8_t MAX_COINS = 8;
static const float PROMPT_RADIUS = 7;
static const float PROMPT_SWELL = 2;
static const float PROMPT_HZ = 1.5f;

static const int16_t CENTER_X = 120;
static const int16_t LEFT_X = 8;
static const int16_t RIGHT_X = 232;

static uint16_t bands[2][ARENA_PX * ArenaRenderer::BAND_H];

PlayScreen::PlayScreen(Round &round, const Profile &profile, SoundManager &sound, ScreenManager &screenManager,
                       Screen &shop, Screen &settings, void (*died)())
    : titleText(CENTER_X, 10, "JackHammer", Theme::TEXT, 3, TC_DATUM),
      titleBest(LEFT_X, 46, "", Theme::TEXT_2, 1, TL_DATUM),
      titleCoins(RIGHT_X, 46, "", Theme::GOLD, 1, TR_DATUM),
      shopHint(LEFT_X, 62, "^ Shop", Theme::TEXT_DIM, 1, TL_DATUM),
      settingsHint(RIGHT_X, 62, "Settings v", Theme::TEXT_DIM, 1, TR_DATUM),
      multiplierText(LEFT_X, 20, "", Theme::TEXT_2, 2, TL_DATUM),
      scoreText(CENTER_X, 8, "", Theme::TEXT, 4, TC_DATUM),
      coinText(RIGHT_X, 20, "", Theme::GOLD, 2, TR_DATUM),
      bestText(CENTER_X, 50, "", Theme::TEXT_DIM, 1, TC_DATUM),
      round(round),
      profile(profile),
      sound(sound),
      screenManager(screenManager),
      shop(shop),
      settings(settings),
      died(died)
{
    addElement(&titleText);
    addElement(&titleBest);
    addElement(&titleCoins);
    addElement(&shopHint);
    addElement(&settingsHint);
    addElement(&multiplierText);
    addElement(&scoreText);
    addElement(&coinText);
    addElement(&bestText);
    effects.reset();
}

void PlayScreen::startTitleParticles()
{
    effects.reset();
    effects.setSource(style().start, ARENA_PX / 2, ARENA_PX / 2);
}

void PlayScreen::toTitle()
{
    mode = Mode::TITLE;
    // A fresh field for the hammer to stand in; it only steps once a round starts
    round.start(esp_random());
    startTitleParticles();
    frameDue = true;
    forceRefresh();
}

void PlayScreen::newRound()
{
    round.start(esp_random());
    // The Unity StyleManager destroyed the start particles as the round began
    effects.reset();
    nextBackground();
    mode = Mode::PLAY;
    sound.playStart();
    frameDue = true;
    forceRefresh();
}

void PlayScreen::styleChanged()
{
    backgroundIndex = 0;
    if (mode == Mode::TITLE)
    {
        startTitleParticles();
    }
    frameDue = true;
}

void PlayScreen::nextBackground()
{
    uint8_t count = style().backgroundCount;
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
    static const int8_t DX[SIDE_COUNT] = {0, 1, 0, -1};
    static const int8_t DY[SIDE_COUNT] = {-1, 0, 1, 0};
    const Emitter &death = *style().death;
    for (uint8_t i = 0; i < round.getMarkCount(); i++)
    {
        const Round::Mark &m = round.getMark(i);
        float x = ARENA_PX / 2 + DX[m.side] * m.distance * PX_PER_UNIT;
        float y = ARENA_PX / 2 + DY[m.side] * m.distance * PX_PER_UNIT;
        effects.emit(death, x, y, death.burst);
        if (!(events & EVENT_DEAD))
        {
            uint32_t coins = round.getMultiplier();
            effects.coins(x, y, (uint8_t)(coins < MAX_COINS ? coins : MAX_COINS));
        }
    }
    round.clearMarks();

    if (events & EVENT_DEAD)
    {
        effects.emit(death, ARENA_PX / 2, ARENA_PX / 2, 2 * death.burst);
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
    if (steps > 0 && (mode != Mode::DYING || style().player->frames > 1))
    {
        frameDue = true;
    }

    // Long enough for the burst to be seen; the particles that fly on are cut by the modal
    if (mode == Mode::DYING && !deathReported && now - dyingSince >= DYING_MS)
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
    bool title = mode == Mode::TITLE;
    titleText.setVisible(title);
    titleBest.setVisible(title);
    titleCoins.setVisible(title);
    shopHint.setVisible(title);
    settingsHint.setVisible(title);
    multiplierText.setVisible(!title);
    scoreText.setVisible(!title);
    coinText.setVisible(!title);
    bestText.setVisible(!title);

    char buf[24];
    if (title)
    {
        snprintf(buf, sizeof(buf), "Best %lu", (unsigned long)profile.best);
        titleBest.setText(buf);
        snprintf(buf, sizeof(buf), "%lu coins", (unsigned long)profile.coins);
        titleCoins.setText(buf);
        return;
    }
    snprintf(buf, sizeof(buf), "x%lu", (unsigned long)round.getMultiplier());
    multiplierText.setText(buf);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)round.getPoints());
    scoreText.setText(buf);
    // The round's coins show as they come, though they are banked at its end
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)(profile.coins + round.getUnbanked()));
    coinText.setText(buf);
    uint32_t best = round.getPoints() > profile.best ? round.getPoints() : profile.best;
    snprintf(buf, sizeof(buf), "BEST %lu", (unsigned long)best);
    bestText.setText(buf);
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
    const StyleArt &art = style();
    uint8_t prompt = 0;
    if (mode == Mode::TITLE)
    {
        float swell = (sinf(tick * (6.2832f * PROMPT_HZ / 60)) + 1) / 2;
        prompt = (uint8_t)(PROMPT_RADIUS + PROMPT_SWELL * swell + 0.5f);
    }
    ArenaRenderer::Scene scene = {&art,
                                  art.backgrounds[backgroundIndex % art.backgroundCount],
                                  &round,
                                  &effects,
                                  tick,
                                  !(mode == Mode::DYING && round.isDead()),
                                  prompt,
                                  (uint16_t)Theme::CONFIRM};
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
