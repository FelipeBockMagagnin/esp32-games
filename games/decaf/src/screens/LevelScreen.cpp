#include "LevelScreen.h"

static const int16_t HUD_H = 16;
static const int16_t PLAY_TOP = HUD_H;
static const int16_t CENTER_X = DISPLAY_W / 2;

static const unsigned long INTRO_MS = 2200;
static const unsigned long TALK_GUARD_MS = 400; // A jump held into the coworker must not skip her first line
static const unsigned long OUTRO_GUARD_MS = 800;
static const uint16_t STEP_THIRDS = 50; // 1/60 s in thirds of a millisecond
static const uint8_t MAX_STEPS_PER_FRAME = 4;
static const unsigned long MAX_CATCH_UP_MS = 100;

static const int16_t TALK_X = 8;
static const int16_t TALK_Y = 20;
static const int16_t TALK_W = DISPLAY_W - 2 * TALK_X;
static const int16_t TALK_H = 72;
// The dialogue box covers the top bands, so only the rest of the floor keeps moving under it
static const uint8_t TALK_FIRST_BAND = (TALK_Y + TALK_H - PLAY_TOP) / Renderer::BAND_H + 1;

static const int16_t PAUSE_W = 120;
static const int16_t PAUSE_H = 112;
static const int16_t PAUSE_X = (DISPLAY_W - PAUSE_W) / 2;
static const int16_t PAUSE_Y = PLAY_TOP + (VIEW_H - PAUSE_H) / 2;
static const char *const PAUSE_LABELS[] = {"Resume", "Restart", "Settings", "Quit"};

static const int16_t CARD_Y[] = {64, 92, 138, 156, 200};
static const uint8_t CARD_SIZE[] = {2, 4, 1, 1, 1};

// Both double buffers of the play area: one is drawn while DMA sends the other
static uint16_t bands[2][VIEW_W * Renderer::BAND_H];

static void formatTime(char *out, size_t size, uint32_t ticks)
{
    uint32_t tenths = ticks / (STEPS_PER_SECOND / 10);
    snprintf(out, size, "%lu:%02lu.%lu", (unsigned long)(tenths / 600), (unsigned long)(tenths / 10 % 60),
             (unsigned long)(tenths % 10));
}

LevelScreen::LevelScreen(ScreenManager &screenManager, SoundManager &sound, SaveBlob &records,
                         Screen &settingsScreen, void (*quit)())
    : screenManager(screenManager),
      sound(sound),
      records(records),
      settingsScreen(settingsScreen),
      quit(quit),
      mode(Mode::INTRO),
      modeSince(0),
      lastStepAt(0),
      stepCredit(0),
      held{false, false, false},
      jumpLatch(false),
      dashLatch(false),
      frameDue(false),
      clearPlayArea(false),
      pauseRow(0),
      page(0),
      newBest(false),
      hudBar(0, 0, DISPLAY_W, HUD_H, Theme::HEADER, true, Theme::HEADER),
      floorText(4, 4, "", Theme::GOLD),
      timeText(CENTER_X, 4, "", Theme::TEXT, 1, TC_DATUM),
      bossText(236, 4, "", Theme::BAD, 1, TR_DATUM),
      deathText(DISPLAY_W - 4, 4, "", Theme::TEXT_2, 1, TR_DATUM),
      pausePanel(PAUSE_X, PAUSE_Y, PAUSE_W, PAUSE_H, Theme::FRAME, true, Theme::CARD, true, 4),
      talkBox(TALK_X, TALK_Y, TALK_W, TALK_H, Theme::GOLD, true, Theme::CARD, true, 4),
      speakerText(TALK_X + 10, TALK_Y + 6, "BEA, ACCOUNTS", Theme::GOLD),
      talkLines{Text(TALK_X + 10, TALK_Y + 20, "", Theme::TEXT, 2),
                Text(TALK_X + 10, TALK_Y + 40, "", Theme::TEXT, 2)},
      talkHint(TALK_X + TALK_W - 8, TALK_Y + TALK_H - 12, "Confirm", Theme::TEXT_DIM, 1, TR_DATUM)
{
    addElement(&hudBar);
    addElement(&floorText, Theme::HEADER);
    addElement(&timeText, Theme::HEADER);
    addElement(&bossText, Theme::HEADER);
    addElement(&deathText, Theme::HEADER);

    for (uint8_t i = 0; i < CARD_LINES; i++)
    {
        cardLines[i] = Text(CENTER_X, CARD_Y[i], "", Theme::TEXT, CARD_SIZE[i], TC_DATUM);
        cardLines[i].setVisible(false);
        addElement(&cardLines[i]);
    }

    pausePanel.setVisible(false);
    addElement(&pausePanel);
    for (uint8_t i = 0; i < PAUSE_ROWS; i++)
    {
        pauseRows[i] = Text(CENTER_X, PAUSE_Y + 14 + i * 24, PAUSE_LABELS[i], Theme::TEXT, 2, TC_DATUM);
        pauseRows[i].setVisible(false);
        addElement(&pauseRows[i], Theme::CARD);
    }

    talkBox.setVisible(false);
    addElement(&talkBox);
    speakerText.setVisible(false);
    addElement(&speakerText, Theme::CARD);
    for (Text &line : talkLines)
    {
        line.setVisible(false);
        addElement(&line, Theme::CARD);
    }
    talkHint.setVisible(false);
    addElement(&talkHint, Theme::CARD);
}

void LevelScreen::lift(UI &element)
{
    element.resetDrawn();
    element.setVisible(false);
}

void LevelScreen::begin(uint8_t level)
{
    stage.load(level);
    held[0] = held[1] = held[2] = false;
    jumpLatch = dashLatch = false;
    frameDue = false;
    showPause(false);
    showTalk(false);
    showCard(true);
    mode = Mode::INTRO;
    modeSince = 0;

    const LevelDef &def = stage.getDef();
    char text[24];
    snprintf(text, sizeof(text), "%s %s", def.floor, def.name);
    floorText.setText(text);
    refreshHud();
}

void LevelScreen::setHeld(bool left, bool right, bool jump)
{
    held[0] = left;
    held[1] = right;
    held[2] = jump;
}

void LevelScreen::setMode(Mode m, unsigned long now)
{
    mode = m;
    modeSince = now;
    if (m == Mode::PLAY)
    {
        lastStepAt = now;
        stepCredit = 0;
        frameDue = true;
    }
}

void LevelScreen::showCard(bool intro)
{
    const LevelDef &def = stage.getDef();
    char text[3][48];
    if (intro)
    {
        snprintf(text[0], sizeof(text[0]), "FLOOR %s", def.floor);
        cardLines[0].setColor(Theme::TEXT_2);
        cardLines[0].setText(text[0]);
        cardLines[1].setColor(Theme::GOLD);
        cardLines[1].setText(def.name);
        cardLines[2].setColor(Theme::TEXT);
        cardLines[2].setText(def.circle);
        cardLines[3].setColor(Theme::TEXT_2);
        cardLines[3].setText("Find a coffee machine that works.");
        cardLines[4].setColor(Theme::TEXT_DIM);
        cardLines[4].setText("Confirm jump - Down dash - Up pause");
    }
    else
    {
        const FloorRecord &record = records.floors[stage.getLevel()];
        snprintf(text[0], sizeof(text[0]), "%s %s CLEARED", def.floor, def.name);
        cardLines[0].setColor(Theme::TEXT);
        cardLines[0].setText(text[0]);
        formatTime(text[1], sizeof(text[1]), stage.getTicks());
        cardLines[1].setColor(newBest ? Theme::GOLD : Theme::TEXT);
        cardLines[1].setText(text[1]);
        if (newBest)
        {
            snprintf(text[2], sizeof(text[2]), "New best time!  %u deaths", (unsigned)stage.getDeaths());
        }
        else
        {
            char best[12];
            formatTime(best, sizeof(best), record.bestTicks);
            snprintf(text[2], sizeof(text[2]), "Best %s  -  %u deaths", best, (unsigned)stage.getDeaths());
        }
        cardLines[2].setColor(newBest ? Theme::GOOD : Theme::TEXT_2);
        cardLines[2].setText(text[2]);
        uint8_t next = stage.getLevel() + 1;
        if (next < LEVEL_COUNT)
        {
            snprintf(text[1], sizeof(text[1]), "Next: %s %s", LEVELS[next].floor, LEVELS[next].name);
        }
        else if (next < CIRCLE_COUNT)
        {
            snprintf(text[1], sizeof(text[1]), "%s %s is still being built", CIRCLES[next].floor,
                     CIRCLES[next].name);
        }
        else
        {
            snprintf(text[1], sizeof(text[1]), "There is no floor below. No coffee.");
        }
        cardLines[3].setColor(Theme::INFO);
        cardLines[3].setText(text[1]);
        cardLines[4].setColor(Theme::TEXT_DIM);
        cardLines[4].setText("Confirm: back to the lobby");
    }
    for (Text &line : cardLines)
    {
        line.setVisible(true);
    }
}

void LevelScreen::showPause(bool show)
{
    if (show)
    {
        pausePanel.setVisible(true);
        for (Text &row : pauseRows)
        {
            row.setVisible(true);
        }
        refreshPause();
        return;
    }
    lift(pausePanel);
    for (Text &row : pauseRows)
    {
        lift(row);
    }
}

void LevelScreen::refreshPause()
{
    for (uint8_t i = 0; i < PAUSE_ROWS; i++)
    {
        pauseRows[i].setColor(i == pauseRow ? Theme::SELECT : Theme::TEXT_DIM);
    }
}

void LevelScreen::showTalk(bool show)
{
    if (show)
    {
        talkBox.setVisible(true);
        speakerText.setVisible(true);
        talkHint.setVisible(true);
        for (Text &line : talkLines)
        {
            line.setVisible(true);
        }
        return;
    }
    lift(talkBox);
    lift(speakerText);
    lift(talkHint);
    for (Text &line : talkLines)
    {
        lift(line);
    }
}

void LevelScreen::showPage()
{
    const LevelDef &def = stage.getDef();
    for (uint8_t i = 0; i < 2; i++)
    {
        uint8_t line = page * 2 + i;
        talkLines[i].setText(line < def.dialogueLines ? def.dialogue[line] : "");
    }
    sound.playTalk(page);
}

void LevelScreen::refreshHud()
{
    char text[24];
    formatTime(text, sizeof(text), stage.getTicks());
    timeText.setText(text);
    snprintf(text, sizeof(text), "DEATHS %u", (unsigned)stage.getDeaths());
    deathText.setText(text);
    if (stage.isBossFight())
    {
        uint8_t left = stage.getBossHitsLeft();
        snprintf(text, sizeof(text), "BOSS %.*s", left, "\x03\x03\x03\x03\x03");
        bossText.setText(text);
    }
    else
    {
        bossText.setText("");
    }
}

void LevelScreen::playEvents(uint16_t events)
{
    if (events & EV_DIE)
        sound.playDie();
    else if (events & EV_BOSS_DOWN)
        sound.playBossDown();
    else if (events & EV_BOSS_HIT)
        sound.playBossHit();
    else if (events & EV_BOSS_WAKE)
        sound.playBossWake();
    else if (events & EV_STOMP)
        sound.playStomp();
    else if (events & EV_CHECKPOINT)
        sound.playCheckpoint();
    else if (events & EV_BOSS_THUD)
        sound.playBossThud();
    else if (events & EV_BOSS_CHARGE)
        sound.playBossCharge();
    else if (events & EV_DASH)
        sound.playDash();
    else if (events & EV_JUMP)
        sound.playJump();
    else if (events & EV_SHOT)
        sound.playShot();
    else if (events & EV_CRACK)
        sound.playCrack();
}

void LevelScreen::finishFloor(unsigned long now)
{
    stage.finish();
    newBest = recordClear(records, stage.getLevel(), stage.getTicks(), stage.getDeaths());
    if (!saveGame(records))
    {
        Serial.println("Save failed");
    }
    sound.playFloorDone();
    showTalk(false);
    clearPlayArea = true;
    showCard(false);
    setMode(Mode::OUTRO, now);
}

void LevelScreen::onEnter(TFT_eSPI &tft)
{
    unsigned long now = millis();
    lastStepAt = now;
    stepCredit = 0;
    frameDue = mode == Mode::PLAY || mode == Mode::PAUSE || mode == Mode::TALK;
    Screen::onEnter(tft);
}

void LevelScreen::update(unsigned long now)
{
    if (modeSince == 0)
    {
        modeSince = now;
    }

    if (mode == Mode::INTRO && now - modeSince >= INTRO_MS)
    {
        for (Text &line : cardLines)
        {
            lift(line);
        }
        setMode(Mode::PLAY, now);
    }

    if (mode == Mode::PLAY || mode == Mode::TALK)
    {
        unsigned long elapsed = now - lastStepAt;
        lastStepAt = now;
        stepCredit += (elapsed < MAX_CATCH_UP_MS ? elapsed : MAX_CATCH_UP_MS) * 3;
        uint8_t steps = 0;
        while (stepCredit >= STEP_THIRDS && steps < MAX_STEPS_PER_FRAME)
        {
            Input input = {};
            if (mode == Mode::PLAY)
            {
                input = {held[0], held[1], held[2], jumpLatch, dashLatch};
                jumpLatch = dashLatch = false;
            }
            stage.step(input);
            playEvents(stage.takeEvents());
            stepCredit -= STEP_THIRDS;
            steps++;
        }
        if (steps == MAX_STEPS_PER_FRAME)
        {
            stepCredit = 0;
        }
        if (steps > 0)
        {
            frameDue = true;
        }
        if (mode == Mode::PLAY && stage.getPhase() == Stage::Phase::TALKING)
        {
            setMode(Mode::TALK, now);
            page = 0;
            showTalk(true);
            showPage();
        }
    }

    if (refreshDue(now))
    {
        refreshHud();
    }
}

void LevelScreen::render(TFT_eSPI &tft)
{
    if (clearPlayArea)
    {
        tft.fillRect(0, PLAY_TOP, DISPLAY_W, VIEW_H, (uint16_t)bgColor);
        clearPlayArea = false;
    }
    if (frameDue && (mode == Mode::PLAY || mode == Mode::PAUSE || mode == Mode::TALK))
    {
        pushFrame(tft);
        frameDue = false;
    }
    Screen::render(tft);
}

void LevelScreen::pushFrame(TFT_eSPI &tft)
{
    renderer.beginFrame(stage);
    uint8_t first = mode == Mode::TALK && talkBox.isVisible() ? TALK_FIRST_BAND : 0;
    tft.startWrite();
    for (uint8_t b = first; b < Renderer::BAND_COUNT; b++)
    {
        uint16_t *buffer = bands[b & 1];
        renderer.renderBand(stage, b, buffer);
        // Waits for the previous band's transfer, which ran while this one was drawn
        tft.pushImageDMA(0, PLAY_TOP + b * Renderer::BAND_H, VIEW_W, Renderer::BAND_H, buffer);
    }
    tft.dmaWait();
    tft.endWrite();
}

void LevelScreen::onConfirmPress()
{
    unsigned long now = millis();
    switch (mode)
    {
    case Mode::INTRO:
        for (Text &line : cardLines)
        {
            lift(line);
        }
        setMode(Mode::PLAY, now);
        break;
    case Mode::PLAY:
        jumpLatch = true;
        break;
    case Mode::PAUSE:
        sound.playMenu();
        switch (pauseRow)
        {
        case 0:
            showPause(false);
            setMode(Mode::PLAY, now);
            break;
        case 1:
            begin(stage.getLevel());
            screenManager.repaint();
            break;
        case 2:
            screenManager.openModal(&settingsScreen);
            break;
        case 3:
            quit();
            break;
        }
        break;
    case Mode::TALK:
        if (now - modeSince < TALK_GUARD_MS)
        {
            break;
        }
        page++;
        if (page * 2 >= stage.getDef().dialogueLines)
        {
            finishFloor(now);
        }
        else
        {
            showPage();
        }
        break;
    case Mode::OUTRO:
        if (now - modeSince >= OUTRO_GUARD_MS)
        {
            sound.playMenu();
            quit();
        }
        break;
    }
}

void LevelScreen::onUpPress()
{
    if (mode == Mode::PLAY)
    {
        sound.playMenu();
        pauseRow = 0;
        setMode(Mode::PAUSE, millis());
        showPause(true);
    }
    else if (mode == Mode::PAUSE)
    {
        pauseRow = pauseRow > 0 ? pauseRow - 1 : PAUSE_ROWS - 1;
        refreshPause();
        sound.playMenu();
    }
}

void LevelScreen::onDownPress()
{
    if (mode == Mode::PLAY)
    {
        dashLatch = true;
    }
    else if (mode == Mode::PAUSE)
    {
        pauseRow = (pauseRow + 1) % PAUSE_ROWS;
        refreshPause();
        sound.playMenu();
    }
}

bool LevelScreen::onBackPress()
{
    if (mode == Mode::PAUSE)
    {
        showPause(false);
        setMode(Mode::PLAY, millis());
    }
    return true;
}
