#include "PlayScreen.h"
#include <ctype.h>
#include "../ui/CellArt.h"

static const int16_t FIELD_X = 10;
static const int16_t FIELD_Y = 34;
static const int16_t FIELD_W = Snake::COLS * CellArt::SIZE;
static const int16_t FIELD_H = Snake::ROWS * CellArt::SIZE;

static const int16_t BOX_X = 30;
static const int16_t BOX_Y = 116;
static const int16_t BOX_W = 180;
static const int16_t BOX_H = 116;

static const unsigned long PULSE_MS = 350;
static const unsigned long BLINK_MS = 140;
static const unsigned long FLASH_MS = 130;
static const uint8_t FLASH_COUNT = 8;

PlayScreen::PlayScreen(TFT_eSPI &tft, Snake &snake, Records &records, RandomSource random, void (*save)(),
                       ScreenManager &screenManager, SoundManager &sound)
    : tft(tft),
      snake(snake),
      records(records),
      random(random),
      save(save),
      screenManager(screenManager),
      sound(sound),
      scoreText(8, 8, "0000", Theme::TEXT, 2, TL_DATUM),
      levelText(120, 11, "", Theme::TEXT_2, 1, TC_DATUM),
      bestText(232, 8, "", Theme::TEXT_2, 2, TR_DATUM)
{
    addElement(&scoreText);
    addElement(&levelText);
    addElement(&bestText);
}

void PlayScreen::onEnter(TFT_eSPI &display)
{
    newGame();
    Screen::onEnter(display);
    drawBorder();
    drawField();
}

void PlayScreen::newGame()
{
    snake.reset(records.level, random);
    state = State::READY;
    choice = 0;
    won = false;
    newBest = false;
    eyesShut = false;
    snakeShown = true;
    nextBlink = millis() + 2000;
    levelText.setText("READY");
    refreshScore();
}

void PlayScreen::refreshScore()
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%04u", (unsigned)snake.getScore());
    scoreText.setText(buf);
    uint16_t best = records.best[snake.getLevel()];
    if (snake.getScore() > best)
    {
        best = snake.getScore();
    }
    snprintf(buf, sizeof(buf), "HI %04u", (unsigned)best);
    bestText.setText(buf);
}

void PlayScreen::showLevel()
{
    char name[8];
    const char *src = LEVELS[snake.getLevel()].name;
    uint8_t i = 0;
    for (; src[i] != '\0' && i < sizeof(name) - 1; i++)
    {
        name[i] = (char)toupper(src[i]);
    }
    name[i] = '\0';
    levelText.setText(name);
}

void PlayScreen::cellOrigin(Cell c, int16_t &px, int16_t &py) const
{
    px = FIELD_X + c.x * CellArt::SIZE;
    py = FIELD_Y + c.y * CellArt::SIZE;
}

// Solid walls kill; the dashed ones of a wrapping level let the snake through
void PlayScreen::drawBorder()
{
    int16_t x = FIELD_X - 4, y = FIELD_Y - 4, w = FIELD_W + 8, h = FIELD_H + 8;
    tft.fillRect(x, y, w, h, Theme::BG);
    tft.fillRect(x + 2, y + 2, w - 4, h - 4, Theme::FIELD);
    uint16_t ink = Theme::INK;
    if (!LEVELS[snake.getLevel()].wrap)
    {
        tft.fillRect(x, y, w, 2, ink);
        tft.fillRect(x, y + h - 2, w, 2, ink);
        tft.fillRect(x, y, 2, h, ink);
        tft.fillRect(x + w - 2, y, 2, h, ink);
        return;
    }
    for (int16_t i = 0; i < w; i += 8)
    {
        tft.fillRect(x + i, y, 4, 2, ink);
        tft.fillRect(x + i, y + h - 2, 4, 2, ink);
    }
    for (int16_t i = 0; i < h; i += 8)
    {
        tft.fillRect(x, y + i, 2, 4, ink);
        tft.fillRect(x + w - 2, y + i, 2, 4, ink);
    }
}

void PlayScreen::drawField()
{
    for (uint8_t y = 0; y < Snake::ROWS; y++)
    {
        for (uint8_t x = 0; x < Snake::COLS; x++)
        {
            Cell c = {x, y};
            if (!snake.occupied(c))
            {
                drawEmpty(c);
            }
        }
    }
    drawSnake(snakeShown);
    drawFood();
}

void PlayScreen::drawEmpty(Cell c)
{
    int16_t px, py;
    cellOrigin(c, px, py);
    CellArt::empty(tft, px, py);
}

void PlayScreen::drawFood()
{
    if (!snake.hasFood())
    {
        return;
    }
    int16_t px, py;
    cellOrigin(snake.food(), px, py);
    CellArt::food(tft, px, py, foodBig);
}

void PlayScreen::drawSegment(uint16_t i)
{
    int16_t px, py;
    Cell c = snake.segment(i);
    cellOrigin(c, px, py);
    if (i == 0)
    {
        Cell next;
        bool mouthOpen = state != State::DYING && state != State::OVER && snake.hasFood() && snake.ahead(next) &&
                         next == snake.food();
        bool shut = eyesShut || state == State::DYING || state == State::OVER;
        CellArt::head(tft, px, py, snake.heading(), mouthOpen, shut);
    }
    else if (i == snake.length() - 1)
    {
        CellArt::tail(tft, px, py, snake.entered(i - 1));
    }
    else
    {
        uint8_t joins = CellArt::joinBit(snake.entered(i - 1)) | CellArt::joinBit(opposite(snake.entered(i)));
        CellArt::body(tft, px, py, joins, snake.fat(i), ((c.x + c.y) & 1) != 0);
    }
}

void PlayScreen::drawSnake(bool shown)
{
    for (uint16_t i = 0; i < snake.length(); i++)
    {
        if (shown)
        {
            drawSegment(i);
        }
        else
        {
            drawEmpty(snake.segment(i));
        }
    }
}

void PlayScreen::drawBox()
{
    uint16_t ink = Theme::INK, field = Theme::FIELD;
    tft.fillRect(BOX_X, BOX_Y, BOX_W, BOX_H, field);
    tft.fillRect(BOX_X, BOX_Y, BOX_W, 2, ink);
    tft.fillRect(BOX_X, BOX_Y + BOX_H - 2, BOX_W, 2, ink);
    tft.fillRect(BOX_X, BOX_Y, 2, BOX_H, ink);
    tft.fillRect(BOX_X + BOX_W - 2, BOX_Y, 2, BOX_H, ink);
    tft.drawRect(BOX_X + 4, BOX_Y + 4, BOX_W - 8, BOX_H - 8, ink);

    int16_t cx = BOX_X + BOX_W / 2;
    tft.setTextColor(ink);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    const char *title = state == State::PAUSED ? "PAUSED" : (won ? "YOU WIN!" : "GAME OVER");
    tft.drawString(title, cx, BOX_Y + 14);

    char line[24];
    tft.setTextSize(1);
    snprintf(line, sizeof(line), "SCORE %u", (unsigned)snake.getScore());
    tft.drawString(line, cx, BOX_Y + 38);
    if (newBest)
    {
        tft.drawString("NEW BEST!", cx, BOX_Y + 50);
    }
    else
    {
        snprintf(line, sizeof(line), "BEST %u", (unsigned)records.best[snake.getLevel()]);
        tft.drawString(line, cx, BOX_Y + 50);
    }

    const char *options[2] = {state == State::PAUSED ? "RESUME" : "AGAIN", state == State::PAUSED ? "QUIT" : "MENU"};
    tft.setTextSize(2);
    tft.setTextDatum(TL_DATUM);
    for (uint8_t i = 0; i < 2; i++)
    {
        int16_t y = BOX_Y + 68 + i * 22;
        tft.drawString(options[i], BOX_X + 52, y);
        if (i == choice)
        {
            tft.fillTriangle(BOX_X + 32, y, BOX_X + 32, y + 13, BOX_X + 39, y + 6, ink);
        }
    }
}

void PlayScreen::update(unsigned long now)
{
    if (state == State::READY || state == State::PLAYING)
    {
        if (now - lastPulse >= PULSE_MS)
        {
            lastPulse = now;
            foodBig = !foodBig;
            drawFood();
        }
        if ((long)(now - nextBlink) >= 0)
        {
            eyesShut = !eyesShut;
            nextBlink = now + (eyesShut ? BLINK_MS : 1500 + random() % 2500);
            drawSegment(0);
        }
    }

    switch (state)
    {
    case State::READY:
        if (refreshDue(now))
        {
            levelText.setText((now / 400) % 2 ? "READY" : "");
        }
        break;
    case State::PLAYING:
        if (now - lastStep >= snake.getStepMs())
        {
            lastStep = now - lastStep < 2u * snake.getStepMs() ? lastStep + snake.getStepMs() : now;
            advance(now);
        }
        break;
    case State::DYING:
        if (now - lastFlash >= FLASH_MS)
        {
            lastFlash = now;
            snakeShown = !snakeShown;
            drawSnake(snakeShown);
            if (++flashes >= FLASH_COUNT)
            {
                finish();
            }
        }
        break;
    default:
        break;
    }
}

void PlayScreen::advance(unsigned long now)
{
    uint16_t oldLength = snake.length();
    Snake::Result result = snake.step();
    if (result == Snake::DIED)
    {
        die();
        return;
    }

    Cell freed;
    if (snake.vacated(freed))
    {
        drawEmpty(freed);
    }
    drawSegment(0);
    drawSegment(1);
    if (snake.length() == oldLength)
    {
        drawSegment(snake.length() - 1);
    }

    if (result == Snake::ATE || result == Snake::WON)
    {
        sound.playEat();
        refreshScore();
    }
    if (result == Snake::ATE)
    {
        drawFood();
    }
    else if (result == Snake::WON)
    {
        won = true;
        die();
    }
}

void PlayScreen::die()
{
    if (!won)
    {
        sound.playDie();
    }
    state = State::DYING;
    flashes = 0;
    lastFlash = millis();
    eyesShut = true;
    drawSegment(0);
    uint16_t &best = records.best[snake.getLevel()];
    if (snake.getScore() > best)
    {
        best = snake.getScore();
        newBest = true;
        save();
    }
}

void PlayScreen::finish()
{
    state = State::OVER;
    snakeShown = true;
    drawSnake(true);
    choice = 0;
    showLevel();
    if (newBest || won)
    {
        sound.playRecord();
    }
    drawBox();
}

void PlayScreen::press(Dir d)
{
    if (state == State::READY)
    {
        state = State::PLAYING;
        lastStep = millis();
        showLevel();
        sound.playStart();
    }
    if (state == State::PLAYING)
    {
        snake.turn(d);
    }
}

void PlayScreen::onUpPress()
{
    if (state == State::PAUSED || state == State::OVER)
    {
        choice ^= 1;
        sound.playMenu();
        drawBox();
        return;
    }
    press(Dir::UP);
}

void PlayScreen::onDownPress()
{
    if (state == State::PAUSED || state == State::OVER)
    {
        onUpPress();
        return;
    }
    press(Dir::DOWN);
}

bool PlayScreen::onBackPress()
{
    press(Dir::LEFT);
    return true;
}

bool PlayScreen::onRightPress()
{
    press(Dir::RIGHT);
    return true;
}

void PlayScreen::onConfirmPress()
{
    switch (state)
    {
    case State::READY:
        press(snake.heading());
        break;
    case State::PLAYING:
        state = State::PAUSED;
        choice = 0;
        levelText.setText("PAUSED");
        sound.playPause();
        drawBox();
        break;
    case State::PAUSED:
        sound.playMenu();
        if (choice == 0)
        {
            state = State::PLAYING;
            lastStep = millis();
            showLevel();
            drawField();
        }
        else
        {
            screenManager.closeModal();
        }
        break;
    case State::OVER:
        sound.playMenu();
        if (choice == 0)
        {
            newGame();
            drawBorder();
            drawField();
        }
        else
        {
            screenManager.closeModal();
        }
        break;
    default:
        break;
    }
}
