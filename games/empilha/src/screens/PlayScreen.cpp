#include "PlayScreen.h"
#include "../ui/BlockArt.h"

static const int16_t WELL_X = 3;
static const int16_t WELL_Y = 3;
static const int16_t WELL_W = COLS * WellView::TILE;
static const int16_t WELL_H = VISIBLE_ROWS * WellView::TILE;

static const int16_t PANEL_X = WELL_X + WELL_W + 5;
static const int16_t PANEL_W = 240 - PANEL_X - 2;
static const int16_t PANEL_CENTER = PANEL_X + PANEL_W / 2;

static const int16_t NEXT_Y = 15;
static const int16_t NEXT_TILE = 9;
static const int16_t NEXT_PITCH = 24;
static const int16_t STAT_Y = 94;
static const int16_t STAT_PITCH = 34;
static const int16_t BEST_Y = STAT_Y + 3 * STAT_PITCH;
static const int16_t CHAIN_Y = BEST_Y + 40;
static const int16_t HINT_Y = 286;
static const int16_t BAR_Y = 310;
static const int16_t BAR_H = 6;

static const int16_t BOX_W = 184;
static const int16_t BOX_H = 128;
static const int16_t BOX_X = (240 - BOX_W) / 2;
static const int16_t BOX_Y = (320 - BOX_H) / 2;

// A long frame, a save or a box closing, must not drop the piece a dozen rows at once
static const unsigned long MAX_TICK_MS = 120;
static const unsigned long WIPE_MS = 28;
static const unsigned long SETTLE_MS = 45;
static const unsigned long END_ROW_MS = 28;
static const unsigned long END_PAUSE_MS = 500;
// The bar stays hidden for a plain press, so turning a piece never flickers it
static const unsigned long PAUSE_HOLD_MS = 700;
static const unsigned long PAUSE_BAR_MS = 200;
// A player mashing as the stack tops out must not pick from the box unseen
static const unsigned long BOX_GUARD_MS = 500;

PlayScreen::PlayScreen(TFT_eSPI &tft, Game &game, Records &records, RandomSource random, void (*save)(),
                       bool (*keyDown)(Key), ScreenManager &screenManager, SoundManager &sound)
    : tft(tft),
      game(game),
      records(records),
      random(random),
      save(save),
      keyDown(keyDown),
      screenManager(screenManager),
      sound(sound),
      view(tft, game),
      nextLabel(PANEL_CENTER, 3, "NEXT", Theme::TEXT_DIM, 1, TC_DATUM),
      bestLabel(PANEL_CENTER, BEST_Y, "BEST", Theme::TEXT_DIM, 1, TC_DATUM),
      bestValue(PANEL_CENTER, BEST_Y + 10, "", Theme::GOLD, 2, TC_DATUM),
      chainText(PANEL_CENTER, CHAIN_Y, "", Theme::GOOD, 2, TC_DATUM),
      hintTop(PANEL_CENTER, HINT_Y, "HOLD OK", Theme::TEXT_DIM, 1, TC_DATUM),
      hintBottom(PANEL_CENTER, HINT_Y + 10, "TO PAUSE", Theme::TEXT_DIM, 1, TC_DATUM)
{
    addElement(&nextLabel);
    for (uint8_t i = 0; i < 3; i++)
    {
        int16_t y = STAT_Y + i * STAT_PITCH;
        statLabels[i] = Text(PANEL_CENTER, y, "", Theme::TEXT_DIM, 1, TC_DATUM);
        statValues[i] = Text(PANEL_CENTER, y + 10, "", Theme::TEXT, 2, TC_DATUM);
        addElement(&statLabels[i]);
        addElement(&statValues[i]);
    }
    addElement(&bestLabel);
    addElement(&bestValue);
    addElement(&chainText);
    addElement(&hintTop);
    addElement(&hintBottom);
    view.place(WELL_X, WELL_Y);
}

void PlayScreen::onEnter(TFT_eSPI &display)
{
    newGame();
    Screen::onEnter(display);
    paint();
}

void PlayScreen::newGame()
{
    mode = records.mode < MODE_COUNT ? (Mode)records.mode : Mode::ENDLESS;
    game.reset(mode, random);
    game.takeEvents();
    state = State::PLAYING;
    wipeStep = 0;
    chainShown = 0;
    softBlocked = keyDown(Key::DOWN);
    pauseHeld = false;
    barShown = 0;
    choice = 0;
    newBest = false;
    recorded = false;
    lastTick = millis();
    uint8_t m = (uint8_t)mode;
    if (records.plays[m] < 0xFFFF)
    {
        records.plays[m]++;
    }
    save();

    bool sprint = mode == Mode::SPRINT;
    statLabels[0].setText(sprint ? "TIME" : "SCORE");
    statLabels[1].setText(sprint ? "LEFT" : "LEVEL");
    statLabels[2].setText(sprint ? "PIECES" : "ROWS");
    chainText.setText("");
    refreshBest();
    forceRefresh();
    refreshPanel();
}

void PlayScreen::refreshBest()
{
    char buf[12];
    uint32_t best = records.best[(uint8_t)mode];
    if (best == 0)
    {
        snprintf(buf, sizeof(buf), "-");
    }
    else if (mode == Mode::SPRINT)
    {
        snprintf(buf, sizeof(buf), "%u:%02u", (unsigned)(best / 600), (unsigned)((best / 10) % 60));
    }
    else
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)best);
    }
    bestValue.setText(buf);
}

void PlayScreen::refreshPanel()
{
    char buf[12];
    if (mode == Mode::SPRINT)
    {
        uint32_t tenths = game.tenths();
        snprintf(buf, sizeof(buf), "%u:%02u", (unsigned)(tenths / 600), (unsigned)((tenths / 10) % 60));
        statValues[0].setText(buf);
        snprintf(buf, sizeof(buf), "%u", (unsigned)(game.rows() >= SPRINT_ROWS ? 0 : SPRINT_ROWS - game.rows()));
        statValues[1].setText(buf);
        snprintf(buf, sizeof(buf), "%u", (unsigned)game.pieces());
        statValues[2].setText(buf);
        return;
    }
    snprintf(buf, sizeof(buf), "%u", (unsigned)game.score());
    statValues[0].setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)game.level());
    statValues[1].setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)game.rows());
    statValues[2].setText(buf);
}

void PlayScreen::paint()
{
    tft.drawRect(WELL_X - 2, WELL_Y - 2, WELL_W + 4, WELL_H + 4, Theme::FRAME);
    tft.drawRect(WELL_X - 1, WELL_Y - 1, WELL_W + 2, WELL_H + 2, Theme::FRAME_DIM);
    view.invalidate();
    view.sync();
    drawNext();
    barShown = 0;
}

void PlayScreen::repaintAll()
{
    Screen::repaint(tft);
    paint();
}

void PlayScreen::drawNext()
{
    for (uint8_t i = 0; i < NEXT_COUNT; i++)
    {
        int16_t y = NEXT_Y + i * NEXT_PITCH;
        tft.fillRect(PANEL_X, y, PANEL_W, NEXT_TILE * 2, Theme::BG);
        uint8_t type = game.next(i);
        Cell cells[PIECE_CELLS];
        Game::cellsOf(type, 0, cells);
        int16_t x = PANEL_CENTER - Game::boxSize(type) * NEXT_TILE / 2;
        // The long bar sits on the second row of its box, which would hang it below its slot
        int16_t top = Game::boxSize(type) == 4 ? y - NEXT_TILE / 2 : y;
        for (const Cell &c : cells)
        {
            BlockArt::block(tft, x + c.x * NEXT_TILE, top + c.y * NEXT_TILE, NEXT_TILE, type, Theme::BG);
        }
    }
}

void PlayScreen::drawBar(int16_t width)
{
    if (width == barShown)
    {
        return;
    }
    if (width < barShown)
    {
        tft.fillRect(PANEL_X, BAR_Y, PANEL_W, BAR_H, Theme::BG);
        barShown = 0;
    }
    tft.fillRect(PANEL_X + barShown, BAR_Y, width - barShown, BAR_H, Theme::SELECT);
    barShown = width;
}

void PlayScreen::shift(int8_t dx)
{
    if (game.move(dx))
    {
        sound.playMove();
    }
}

void PlayScreen::onUpPress()
{
    if (state == State::PAUSED || state == State::OVER)
    {
        choice = (choice + optionCount() - 1) % optionCount();
        sound.playMenu();
        drawBox();
        return;
    }
    if (state == State::PLAYING && game.falling())
    {
        game.hardDrop();
        sound.playSlam();
    }
}

void PlayScreen::onDownPress()
{
    if (state == State::PAUSED || state == State::OVER)
    {
        choice = (choice + 1) % optionCount();
        sound.playMenu();
        drawBox();
    }
}

bool PlayScreen::onBackPress()
{
    if (state == State::PLAYING)
    {
        shift(-1);
    }
    return true;
}

bool PlayScreen::onRightPress()
{
    if (state == State::PLAYING)
    {
        shift(1);
    }
    return true;
}

bool PlayScreen::onRepeat(Key key)
{
    if (state != State::PLAYING || (key != Key::LEFT && key != Key::RIGHT))
    {
        return false;
    }
    shift(key == Key::LEFT ? -1 : 1);
    return true;
}

void PlayScreen::onConfirmPress()
{
    switch (state)
    {
    case State::PLAYING:
        pauseHeld = true;
        pauseHeldAt = millis();
        if (game.rotate())
        {
            sound.playTurn();
        }
        break;
    case State::PAUSED:
        pick();
        break;
    case State::OVER:
        if (millis() - boxAt >= BOX_GUARD_MS)
        {
            pick();
        }
        break;
    default:
        break;
    }
}

void PlayScreen::pause()
{
    state = State::PAUSED;
    pauseHeld = false;
    choice = 0;
    sound.playPause();
    drawBox();
}

void PlayScreen::pick()
{
    sound.playMenu();
    if (state == State::PAUSED && choice == 0)
    {
        state = State::PLAYING;
        lastTick = millis();
        softBlocked = keyDown(Key::DOWN);
        repaintAll();
        return;
    }
    recordRun();
    bool again = state == State::PAUSED ? choice == 1 : choice == 0;
    if (again)
    {
        newGame();
        repaintAll();
        return;
    }
    screenManager.closeModal();
}

// A run left halfway still counts for the best score; Sprint counts only once it is finished
void PlayScreen::recordRun()
{
    if (recorded)
    {
        return;
    }
    recorded = true;
    uint32_t result = mode == Mode::SPRINT ? (game.phase() == Game::Phase::FINISHED ? game.tenths() : 0) : game.score();
    newBest = updateBest(records, mode, result);
    if (newBest)
    {
        save();
    }
}

void PlayScreen::beginEnding(unsigned long now)
{
    state = State::ENDING;
    pauseHeld = false;
    drawBar(0);
    chainText.setText("");
    view.sync();
    endRow = VISIBLE_ROWS - 1;
    lastFrame = now;
    forceRefresh();
    refreshPanel();
    recordRun();
    refreshBest();
    if (game.phase() == Game::Phase::FINISHED)
    {
        sound.playFinished();
    }
    else
    {
        sound.playTopOut();
    }
}

void PlayScreen::handleEvents(unsigned long now)
{
    uint8_t events = game.takeEvents();
    if (events == 0)
    {
        return;
    }
    if ((events & Game::EVENT_LANDED) && !(events & Game::EVENT_ROWS))
    {
        sound.playLand();
    }
    if (events & Game::EVENT_ROWS)
    {
        view.sync();
        wipeStep = 0;
        lastFrame = now - WIPE_MS;
        uint8_t count = 0;
        for (uint32_t rows = game.fullRows(); rows != 0; rows &= rows - 1)
        {
            count++;
        }
        if (game.chain() > 1)
        {
            sound.playChain(game.chain() - 2);
        }
        else
        {
            sound.playRows(count);
        }
        if (game.chain() > 1)
        {
            char buf[6];
            snprintf(buf, sizeof(buf), "x%u", (unsigned)game.chain());
            chainText.setText(buf);
            chainShown = game.chain();
        }
    }
    if (events & Game::EVENT_LEVEL)
    {
        sound.playLevel();
    }
    if (events & Game::EVENT_SPAWNED)
    {
        drawNext();
        softBlocked = keyDown(Key::DOWN);
        if (chainShown > 0)
        {
            chainShown = 0;
            chainText.setText("");
        }
    }
    forceRefresh();
}

void PlayScreen::stepAnimations(unsigned long now)
{
    if (game.phase() == Game::Phase::CLEARING)
    {
        if (now - lastFrame < WIPE_MS)
        {
            return;
        }
        lastFrame = now;
        if (wipeStep < WellView::wipeSteps())
        {
            view.wipe(game.fullRows(), wipeStep++);
            return;
        }
        game.clearRows();
    }
    else if (game.phase() == Game::Phase::SETTLING && now - lastFrame >= SETTLE_MS)
    {
        lastFrame = now;
        game.settleStep();
    }
}

void PlayScreen::checkPauseHold(unsigned long now)
{
    if (!pauseHeld)
    {
        return;
    }
    if (!keyDown(Key::CONFIRM))
    {
        pauseHeld = false;
        drawBar(0);
        return;
    }
    unsigned long held = now - pauseHeldAt;
    if (held >= PAUSE_HOLD_MS)
    {
        drawBar(0);
        pause();
    }
    else if (held >= PAUSE_BAR_MS)
    {
        drawBar((int16_t)((held - PAUSE_BAR_MS) * PANEL_W / (PAUSE_HOLD_MS - PAUSE_BAR_MS)));
    }
}

void PlayScreen::update(unsigned long now)
{
    if (state == State::PAUSED || state == State::OVER)
    {
        return;
    }
    if (state == State::ENDING)
    {
        if (endRow >= 0)
        {
            if (now - lastFrame >= END_ROW_MS)
            {
                lastFrame = now;
                view.fillRow((uint8_t)endRow--, game.phase() == Game::Phase::FINISHED);
            }
        }
        else if (now - lastFrame >= END_PAUSE_MS)
        {
            state = State::OVER;
            boxAt = now;
            choice = 0;
            if (newBest)
            {
                sound.playRecord();
            }
            drawBox();
        }
        return;
    }

    unsigned long tick = now - lastTick;
    lastTick = now;
    if (softBlocked && !keyDown(Key::DOWN))
    {
        softBlocked = false;
    }
    game.advance((uint16_t)(tick > MAX_TICK_MS ? MAX_TICK_MS : tick), keyDown(Key::DOWN) && !softBlocked);
    handleEvents(now);
    stepAnimations(now);
    handleEvents(now);
    if (game.over())
    {
        beginEnding(now);
        return;
    }
    if (game.phase() != Game::Phase::CLEARING)
    {
        view.sync();
    }
    if (game.falling())
    {
        checkPauseHold(now);
    }
    if (refreshDue(now))
    {
        refreshPanel();
    }
}

void PlayScreen::drawBox()
{
    uint16_t card = Theme::CARD, frame = Theme::FRAME, text = Theme::TEXT, text2 = Theme::TEXT_2;
    tft.fillRect(BOX_X, BOX_Y, BOX_W, BOX_H, card);
    tft.drawRect(BOX_X, BOX_Y, BOX_W, BOX_H, frame);
    tft.drawRect(BOX_X + 1, BOX_Y + 1, BOX_W - 2, BOX_H - 2, frame);
    tft.drawRect(BOX_X + 4, BOX_Y + 4, BOX_W - 8, BOX_H - 8, Theme::FRAME_DIM);

    int16_t cx = BOX_X + BOX_W / 2;
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    const char *options[3];
    uint8_t count = optionCount();
    int16_t optionsY;
    if (state == State::PAUSED)
    {
        tft.setTextColor(text);
        tft.drawString("PAUSED", cx, BOX_Y + 14);
        tft.setTextSize(1);
        tft.setTextColor(text2);
        tft.drawString(MODE_NAMES[(uint8_t)mode], cx, BOX_Y + 34);
        options[0] = "RESUME";
        options[1] = "RESTART";
        options[2] = "QUIT";
        optionsY = BOX_Y + 50;
    }
    else
    {
        bool finished = game.phase() == Game::Phase::FINISHED;
        tft.setTextColor(finished ? Theme::GOOD : Theme::BAD);
        tft.drawString(finished ? "FINISHED!" : "TOPPED OUT", cx, BOX_Y + 14);
        tft.setTextSize(1);
        tft.setTextColor(text2);
        char line[28], t[12];
        if (mode == Mode::SPRINT)
        {
            formatTime(game.tenths(), t, sizeof(t));
            snprintf(line, sizeof(line), "TIME %s", t);
            tft.drawString(line, cx, BOX_Y + 36);
            formatTime(records.best[(uint8_t)mode], t, sizeof(t));
            snprintf(line, sizeof(line), "BEST %s", t);
        }
        else
        {
            snprintf(line, sizeof(line), "SCORE %u", (unsigned)game.score());
            tft.drawString(line, cx, BOX_Y + 36);
            snprintf(line, sizeof(line), "BEST %u", (unsigned)records.best[(uint8_t)mode]);
        }
        if (newBest)
        {
            tft.setTextColor(Theme::GOLD);
            tft.drawString("NEW BEST!", cx, BOX_Y + 48);
        }
        else
        {
            tft.drawString(line, cx, BOX_Y + 48);
        }
        tft.setTextColor(text2);
        snprintf(line, sizeof(line), "%u ROWS  %u PIECES", (unsigned)game.rows(), (unsigned)game.pieces());
        tft.drawString(line, cx, BOX_Y + 60);
        options[0] = "AGAIN";
        options[1] = "MENU";
        optionsY = BOX_Y + 76;
    }

    tft.setTextSize(2);
    tft.setTextDatum(TL_DATUM);
    for (uint8_t i = 0; i < count; i++)
    {
        int16_t y = optionsY + i * 22;
        tft.setTextColor(i == choice ? (uint16_t)Theme::SELECT : text);
        tft.drawString(options[i], BOX_X + 56, y);
        if (i == choice)
        {
            tft.fillTriangle(BOX_X + 36, y, BOX_X + 36, y + 13, BOX_X + 43, y + 6, Theme::SELECT);
        }
    }
}
