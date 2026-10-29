#include "PlayScreen.h"
#include "../ui/TileArt.h"

static const int16_t FIELD_Y = 32;
static const int16_t FIELD_H = 320 - FIELD_Y;

static const int16_t BUTTON_X = 100;
static const int16_t BUTTON_Y = 3;
static const int16_t BUTTON_W = 40;
static const int16_t BUTTON_H = 25;

static const int16_t BOX_W = 184;
static const int16_t BOX_H = 128;
static const int16_t BOX_X = (240 - BOX_W) / 2;
static const int16_t BOX_Y = FIELD_Y + (FIELD_H - BOX_H) / 2;

// A tap shorter than HOLD_MS opens; the ring starts filling at RING_MS so a quick tap never
// flickers it
static const unsigned long HOLD_MS = 400;
static const unsigned long RING_MS = 120;
static const unsigned long PULSE_MS = 380;
static const unsigned long REVEAL_MS = 30;
static const unsigned long FLAG_MS = 45;
static const unsigned long BLAST_MS = 70;
static const uint8_t BLAST_FLASHES = 6;
static const unsigned long CHAIN_TOTAL_MS = 1500;
static const unsigned long CHAIN_FASTEST_MS = 25;
static const unsigned long CHAIN_SLOWEST_MS = 90;
static const unsigned long SWEEP_MS = 40;
static const unsigned long SETTLE_MS = 600;

PlayScreen::PlayScreen(TFT_eSPI &tft, Field &field, Records &records, RandomSource random, void (*save)(),
                       bool (*confirmDown)(), ScreenManager &screenManager, SoundManager &sound)
    : tft(tft),
      field(field),
      records(records),
      random(random),
      save(save),
      confirmDown(confirmDown),
      screenManager(screenManager),
      sound(sound),
      view(tft, field),
      minesText(30, 9, "000", Theme::TEXT, 2, TL_DATUM),
      timeText(234, 9, "0:00", Theme::TEXT, 2, TR_DATUM)
{
    addElement(&minesText);
    addElement(&timeText);
}

void PlayScreen::onEnter(TFT_eSPI &display)
{
    newGame();
    Screen::onEnter(display);
    drawHud();
    view.drawAll();
    drawCursorCell();
}

void PlayScreen::newGame()
{
    level = records.level < LEVEL_COUNT ? records.level : 0;
    const Level &lv = LEVELS[level];
    field.reset(lv.cols, lv.rows, lv.mines, random);
    view.place((240 - lv.cols * lv.tile) / 2, FIELD_Y, lv.tile);
    cursor = {(uint8_t)(lv.cols / 2), (uint8_t)(lv.rows / 2)};
    onButton = false;
    state = State::PLAYING;
    holding = false;
    flagAnimating = false;
    timing = false;
    finalTenths = 0;
    newBest = false;
    choice = 0;
    refreshMines();
    timeText.setText("0:00");
}

uint16_t PlayScreen::elapsedTenths(unsigned long now) const
{
    if (!timing)
    {
        return finalTenths;
    }
    unsigned long t = (now - startedAt) / 100;
    return t > TIME_CAP ? TIME_CAP : (uint16_t)t;
}

void PlayScreen::refreshMines()
{
    char buf[8];
    int16_t left = field.minesLeft();
    snprintf(buf, sizeof(buf), left < 0 ? "-%02d" : "%03d", left < 0 ? -left : left);
    minesText.setText(buf);
}

void PlayScreen::refreshTime(unsigned long now)
{
    unsigned s = elapsedTenths(now) / 10;
    char buf[8];
    snprintf(buf, sizeof(buf), "%u:%02u", s / 60, s % 60);
    timeText.setText(buf);
}

void PlayScreen::drawHud()
{
    TileArt::flagGlyph(tft, 2, 3, 26, TileArt::FLAG_FRAMES - 1);
    drawButton();
}

void PlayScreen::drawButton()
{
    bool lit = onButton && state == State::PLAYING;
    tft.fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, 5, Theme::CARD);
    tft.drawRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, 5, lit ? Theme::CURSOR : Theme::FRAME);
    if (lit)
    {
        tft.drawRoundRect(BUTTON_X + 1, BUTTON_Y + 1, BUTTON_W - 2, BUTTON_H - 2, 4, Theme::CURSOR);
    }
    uint16_t ink = lit ? (uint16_t)Theme::CURSOR : (uint16_t)Theme::TEXT;
    int16_t cx = BUTTON_X + BUTTON_W / 2, cy = BUTTON_Y + BUTTON_H / 2;
    if (state == State::PLAYING || state == State::PAUSED)
    {
        tft.fillRect(cx - 6, cy - 6, 4, 13, ink);
        tft.fillRect(cx + 2, cy - 6, 4, 13, ink);
    }
    else if (field.isWon())
    {
        tft.fillTriangle(cx - 8, cy - 2, cx + 8, cy - 2, cx, cy + 4, Theme::GOLD);
        tft.fillTriangle(cx, cy - 9, cx - 5, cy + 8, cx + 1, cy + 1, Theme::GOLD);
        tft.fillTriangle(cx, cy - 9, cx + 5, cy + 8, cx - 1, cy + 1, Theme::GOLD);
    }
    else
    {
        TileArt::mineGlyph(tft, cx, cy, 20);
    }
}

void PlayScreen::drawCursorCell()
{
    view.draw(cursor);
    if (state != State::PLAYING || onButton)
    {
        return;
    }
    int16_t px, py;
    view.cellOrigin(cursor, px, py);
    if (flagAnimating && flagCell == cursor)
    {
        TileArt::flag(tft, px, py, view.tile(), view.parity(cursor), flagFrame);
    }
    TileArt::cursor(tft, px, py, view.tile(), tight);
    if (holding && ringDots > 0)
    {
        TileArt::ring(tft, px, py, view.tile(), ringDots);
    }
}

void PlayScreen::moveTo(Cell c)
{
    if (c.x >= field.cols() || c.y >= field.rows())
    {
        return;
    }
    bool wasButton = onButton;
    view.draw(cursor);
    cursor = c;
    onButton = false;
    if (wasButton)
    {
        drawButton();
    }
    drawCursorCell();
}

void PlayScreen::move(int8_t dx, int8_t dy)
{
    holding = false;
    tight = false;
    lastPulse = millis();
    if (onButton)
    {
        if (dy == 0)
        {
            return;
        }
        onButton = false;
        drawButton();
        cursor.y = dy > 0 ? 0 : field.rows() - 1;
        drawCursorCell();
        sound.playStep();
        return;
    }
    view.draw(cursor);
    if ((dy < 0 && cursor.y == 0) || (dy > 0 && cursor.y == field.rows() - 1))
    {
        onButton = true;
        drawButton();
        sound.playStep();
        return;
    }
    cursor.x = (uint8_t)((cursor.x + field.cols() + dx) % field.cols());
    cursor.y = (uint8_t)(cursor.y + dy);
    drawCursorCell();
    sound.playStep();
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
    if (state == State::PLAYING)
    {
        move(0, -1);
    }
}

void PlayScreen::onDownPress()
{
    if (state == State::PAUSED || state == State::OVER)
    {
        choice = (choice + 1) % optionCount();
        sound.playMenu();
        drawBox();
        return;
    }
    if (state == State::PLAYING)
    {
        move(0, 1);
    }
}

bool PlayScreen::onBackPress()
{
    if (state == State::PLAYING)
    {
        move(-1, 0);
    }
    return true;
}

bool PlayScreen::onRightPress()
{
    if (state == State::PLAYING)
    {
        move(1, 0);
    }
    return true;
}

bool PlayScreen::onRepeat(Key key)
{
    if (state != State::PLAYING)
    {
        return false;
    }
    switch (key)
    {
    case Key::UP:
        move(0, -1);
        break;
    case Key::DOWN:
        move(0, 1);
        break;
    case Key::LEFT:
        move(-1, 0);
        break;
    case Key::RIGHT:
        move(1, 0);
        break;
    default:
        return false;
    }
    return true;
}

void PlayScreen::onConfirmPress()
{
    switch (state)
    {
    case State::PLAYING:
        if (onButton)
        {
            pause();
            return;
        }
        holding = true;
        holdStart = millis();
        ringDots = 0;
        break;
    case State::PAUSED:
    case State::OVER:
        pick();
        break;
    default:
        break;
    }
}

void PlayScreen::pause()
{
    view.finishReveal();
    state = State::PAUSED;
    pausedAt = millis();
    choice = 0;
    sound.playPause();
    drawButton();
    drawBox();
}

void PlayScreen::pick()
{
    sound.playMenu();
    if (state == State::PAUSED && choice == 0)
    {
        state = State::PLAYING;
        startedAt += millis() - pausedAt;
        onButton = false;
        drawButton();
        view.drawAll();
        drawCursorCell();
        return;
    }
    bool again = state == State::PAUSED ? choice == 1 : choice == 0;
    if (again)
    {
        newGame();
        drawButton();
        view.drawAll();
        drawCursorCell();
        return;
    }
    screenManager.closeModal();
}

void PlayScreen::tap(unsigned long now)
{
    view.finishReveal();
    if (field.isFlagged(cursor))
    {
        sound.playDenied();
        return;
    }
    if (!field.armed())
    {
        timing = true;
        startedAt = now;
        if (records.plays[level] < 0xFFFF)
        {
            records.plays[level]++;
        }
        save();
    }
    Field::Result result;
    if (field.isOpen(cursor))
    {
        result = field.chord(cursor);
        if (result == Field::NOTHING)
        {
            sound.playDenied();
            return;
        }
        sound.playChord();
    }
    else
    {
        result = field.open(cursor);
        if (field.openedCount() == 1 && result == Field::OPENED)
        {
            sound.playOpen(field.count(cursor));
        }
    }
    settle(result, now);
}

void PlayScreen::hold()
{
    if (!field.toggleFlag(cursor))
    {
        sound.playDenied();
        drawCursorCell();
        return;
    }
    bool planted = field.isFlagged(cursor);
    sound.playFlag(planted);
    refreshMines();
    flagAnimating = planted;
    flagCell = cursor;
    flagFrame = 0;
    lastFlagFrame = millis();
    drawCursorCell();
}

void PlayScreen::settle(Field::Result result, unsigned long now)
{
    switch (result)
    {
    case Field::OPENED:
        view.beginReveal();
        lastReveal = now - REVEAL_MS;
        break;
    case Field::HIT:
        blast(now);
        break;
    case Field::WON:
        win(now);
        break;
    default:
        break;
    }
}

void PlayScreen::stopClock(unsigned long now)
{
    finalTenths = elapsedTenths(now);
    if (finalTenths == 0)
    {
        finalTenths = 1;
    }
    timing = false;
    refreshTime(now);
}

void PlayScreen::blast(unsigned long now)
{
    stopClock(now);
    view.finishReveal();
    view.drawAll();
    state = State::BLAST;
    holding = false;
    flagAnimating = false;
    frames = 0;
    lastFrame = now;
    sound.playBlast();
    drawButton();

    // The other mines nearest the blast first, then every flag that was wrong
    Cell hit = field.hitCell();
    chainLength = 0;
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            Cell c = {x, y};
            if (field.isMine(c) && !field.isFlagged(c) && !field.isOpen(c))
            {
                chain[chainLength++] = c;
            }
        }
    }
    auto dist = [hit](Cell c)
    { return (c.x - hit.x) * (c.x - hit.x) + (c.y - hit.y) * (c.y - hit.y); };
    for (uint16_t i = 1; i < chainLength; i++)
    {
        Cell c = chain[i];
        uint16_t j = i;
        for (; j > 0 && dist(chain[j - 1]) > dist(c); j--)
        {
            chain[j] = chain[j - 1];
        }
        chain[j] = c;
    }
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            Cell c = {x, y};
            if (field.isFlagged(c) && !field.isMine(c))
            {
                chain[chainLength++] = c;
            }
        }
    }
    chainShown = 0;
    chainMs = chainLength > 0 ? CHAIN_TOTAL_MS / chainLength : CHAIN_SLOWEST_MS;
    chainMs = chainMs < CHAIN_FASTEST_MS ? CHAIN_FASTEST_MS : (chainMs > CHAIN_SLOWEST_MS ? CHAIN_SLOWEST_MS : chainMs);
}

void PlayScreen::win(unsigned long now)
{
    stopClock(now);
    holding = false;
    flagAnimating = false;
    view.beginReveal();
    lastReveal = now - REVEAL_MS;
    state = State::WINNING;
    sweepAt = 0;
    lastFrame = now;
    frames = 0;
    refreshMines();
    drawButton();

    if (records.wins[level] < 0xFFFF)
    {
        records.wins[level]++;
    }
    uint16_t &best = records.best[level];
    if (best == 0 || finalTenths < best)
    {
        best = finalTenths;
        newBest = true;
    }
    save();
    sound.playWin();
}

void PlayScreen::finish()
{
    state = State::OVER;
    choice = 0;
    if (newBest)
    {
        sound.playRecord();
    }
    drawBox();
}

void PlayScreen::update(unsigned long now)
{
    if (view.revealing() && now - lastReveal >= REVEAL_MS)
    {
        lastReveal = now;
        int16_t wave = view.revealStep();
        if (wave >= 0 && field.openedCount() > 1 && state == State::PLAYING)
        {
            sound.playWave((uint16_t)wave);
        }
        if (state == State::PLAYING && !onButton)
        {
            int16_t px, py;
            view.cellOrigin(cursor, px, py);
            TileArt::cursor(tft, px, py, view.tile(), tight);
        }
    }

    switch (state)
    {
    case State::PLAYING:
        if (holding)
        {
            unsigned long held = now - holdStart;
            if (!confirmDown())
            {
                holding = false;
                if (ringDots > 0)
                {
                    drawCursorCell();
                }
                if (held < HOLD_MS)
                {
                    tap(now);
                    if (state == State::PLAYING)
                    {
                        drawCursorCell();
                    }
                }
            }
            else if (held >= HOLD_MS)
            {
                holding = false;
                hold();
            }
            else if (held >= RING_MS)
            {
                uint8_t dots = (uint8_t)((held - RING_MS) * TileArt::RING_DOTS / (HOLD_MS - RING_MS)) + 1;
                if (dots != ringDots)
                {
                    ringDots = dots;
                    int16_t px, py;
                    view.cellOrigin(cursor, px, py);
                    TileArt::ring(tft, px, py, view.tile(), ringDots);
                }
            }
        }
        if (flagAnimating && now - lastFlagFrame >= FLAG_MS)
        {
            lastFlagFrame = now;
            if (++flagFrame >= TileArt::FLAG_FRAMES - 1)
            {
                flagAnimating = false;
            }
            if (flagCell == cursor && !onButton)
            {
                drawCursorCell();
            }
            else
            {
                view.draw(flagCell);
            }
        }
        if (now - lastPulse >= PULSE_MS && !holding)
        {
            lastPulse = now;
            tight = !tight;
            if (!onButton)
            {
                drawCursorCell();
            }
        }
        if (timing && refreshDue(now))
        {
            refreshTime(now);
        }
        break;
    case State::BLAST:
        if (now - lastFrame >= BLAST_MS)
        {
            lastFrame = now;
            int16_t px, py;
            Cell hit = field.hitCell();
            view.cellOrigin(hit, px, py);
            TileArt::mine(tft, px, py, view.tile(), view.parity(hit), true, ++frames % 2 == 1);
            if (frames >= BLAST_FLASHES)
            {
                state = State::CHAIN;
                lastFrame = now;
            }
        }
        break;
    case State::CHAIN:
        if (chainShown < chainLength && now - lastFrame >= chainMs)
        {
            lastFrame = now;
            Cell c = chain[chainShown];
            int16_t px, py;
            view.cellOrigin(c, px, py);
            if (field.isMine(c))
            {
                TileArt::mine(tft, px, py, view.tile(), view.parity(c), false, false);
                sound.playChain((uint8_t)chainShown);
            }
            else
            {
                TileArt::wrongFlag(tft, px, py, view.tile(), view.parity(c));
            }
            chainShown++;
        }
        else if (chainShown >= chainLength && now - lastFrame >= SETTLE_MS)
        {
            finish();
        }
        break;
    case State::WINNING:
        if (view.revealing())
        {
            break;
        }
        if (sweepAt < view.diagonals() + TileArt::FLAG_FRAMES)
        {
            if (now - lastFrame >= SWEEP_MS)
            {
                lastFrame = now;
                view.sweep(sweepAt++);
            }
        }
        else if (now - lastFrame >= SETTLE_MS)
        {
            finish();
        }
        break;
    default:
        break;
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
        tft.drawString(LEVELS[level].name, cx, BOX_Y + 34);
        options[0] = "RESUME";
        options[1] = "RESTART";
        options[2] = "QUIT";
        optionsY = BOX_Y + 50;
    }
    else
    {
        bool won = field.isWon();
        tft.setTextColor(won ? Theme::GOOD : Theme::BAD);
        tft.drawString(won ? "CLEARED!" : "BOOM!", cx, BOX_Y + 14);
        tft.setTextSize(1);
        tft.setTextColor(text2);
        char line[28], t[12];
        if (won)
        {
            formatTime(finalTenths, t, sizeof(t));
            snprintf(line, sizeof(line), "TIME %s", t);
            tft.drawString(line, cx, BOX_Y + 36);
            if (newBest)
            {
                tft.setTextColor(Theme::GOLD);
                tft.drawString("NEW BEST!", cx, BOX_Y + 48);
            }
            else
            {
                formatTime(records.best[level], t, sizeof(t));
                snprintf(line, sizeof(line), "BEST %s", t);
                tft.drawString(line, cx, BOX_Y + 48);
            }
        }
        else
        {
            uint16_t safe = field.cols() * field.rows() - field.mineCount();
            snprintf(line, sizeof(line), "CLEARED %u%%", (unsigned)((safe - field.safeLeft()) * 100u / safe));
            tft.drawString(line, cx, BOX_Y + 36);
            formatTime(finalTenths, t, sizeof(t));
            snprintf(line, sizeof(line), "TIME %s", t);
            tft.drawString(line, cx, BOX_Y + 48);
        }
        tft.setTextColor(text2);
        snprintf(line, sizeof(line), "WON %u OF %u", (unsigned)records.wins[level], (unsigned)records.plays[level]);
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
