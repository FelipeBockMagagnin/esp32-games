#include "TitleScreen.h"
#include "../ui/Themes.h"
#include "../ui/TileArt.h"

static const int16_t DEMO_X = 8;
static const int16_t DEMO_Y = 38;
static const int16_t DEMO_TILE = 16;
static const uint8_t DEMO_MINES = 13;

static const int16_t ROW_X = 5;
static const int16_t PLAY_Y = 146;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t BEST_Y = 250;
static const int16_t BEST_PITCH = 13;

static const unsigned long WAIT_MS = 900;
static const unsigned long REVEAL_MS = 55;
static const unsigned long SOLVE_MS = 140;
static const unsigned long SWEEP_MS = 50;
static const unsigned long REST_MS = 1800;
static const unsigned long COVER_MS = 35;

TitleScreen::TitleScreen(TFT_eSPI &tft, Records &records, RandomSource random, void (*save)(), Settings &settings,
                         void (*settingsChanged)(), ScreenManager &screenManager, SoundManager &sound,
                         Screen &playScreen)
    : tft(tft),
      records(records),
      random(random),
      save(save),
      settings(settings),
      settingsChanged(settingsChanged),
      screenManager(screenManager),
      sound(sound),
      playScreen(playScreen),
      header("Minefield"),
      bestTitle(120, BEST_Y, "BEST TIMES", Theme::TEXT_2, 1, TC_DATUM),
      demoView(tft, demo)
{
    addElement(&header);
    setHeader(&header);
    static const char *const TITLES[ROW_COUNT] = {"Play", "Level", "Theme"};
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, PLAY_Y + i * ROW_PITCH);
        rows[i].setTitle(TITLES[i]);
        addElement(&rows[i]);
    }
    addElement(&bestTitle);
    for (uint8_t i = 0; i < LEVEL_COUNT; i++)
    {
        int16_t y = BEST_Y + 14 + i * BEST_PITCH;
        bestNames[i] = Text(30, y, LEVELS[i].name, Theme::TEXT_2, 1, TL_DATUM);
        bestTimes[i] = Text(150, y, "", Theme::TEXT_2, 1, TR_DATUM);
        bestWins[i] = Text(212, y, "", Theme::TEXT_2, 1, TR_DATUM);
        addElement(&bestNames[i]);
        addElement(&bestTimes[i]);
        addElement(&bestWins[i]);
    }
    demoView.place(DEMO_X, DEMO_Y, DEMO_TILE);
    restartDemo(0);
}

void TitleScreen::refresh()
{
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    rows[1].setValue(LEVELS[records.level].name);
    rows[2].setValue(THEMES[Theme::getActive()].name);
    char buf[16];
    for (uint8_t i = 0; i < LEVEL_COUNT; i++)
    {
        formatTime(records.best[i], buf, sizeof(buf));
        bestTimes[i].setText(buf);
        snprintf(buf, sizeof(buf), "%u won", (unsigned)records.wins[i]);
        bestWins[i].setText(buf);
        Color c = i == records.level ? Theme::TEXT : Theme::TEXT_DIM;
        bestNames[i].setColor(c);
        bestTimes[i].setColor(c);
        bestWins[i].setColor(c);
    }
}

void TitleScreen::onEnter(TFT_eSPI &display)
{
    selected = 0;
    paint(display);
}

void TitleScreen::paint(TFT_eSPI &display)
{
    refresh();
    Screen::onEnter(display);
    tft.fillRect(DEMO_X - 2, DEMO_Y - 2, DEMO_COLS * DEMO_TILE + 4, DEMO_ROWS * DEMO_TILE + 4, Theme::FRAME);
    demoView.drawAll();
}

void TitleScreen::restartDemo(unsigned long now)
{
    demo.reset(DEMO_COLS, DEMO_ROWS, DEMO_MINES, random);
    for (uint8_t i = 0; i < DEMO_CELLS; i++)
    {
        order[i] = i;
    }
    for (uint8_t i = DEMO_CELLS - 1; i > 0; i--)
    {
        uint8_t j = (uint8_t)(random() % (i + 1));
        uint8_t t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    next = 0;
    raising = false;
    phase = Demo::WAITING;
    lastDemo = now;
}

// Opens or flags the next covered cell of the shuffled order, as a player who knows the answers
void TitleScreen::advanceDemo()
{
    while (next < DEMO_CELLS)
    {
        Cell c = {(uint8_t)(order[next] % DEMO_COLS), (uint8_t)(order[next] / DEMO_COLS)};
        next++;
        if (demo.isOpen(c) || demo.isFlagged(c))
        {
            continue;
        }
        if (demo.isMine(c))
        {
            demo.toggleFlag(c);
            int16_t px, py;
            demoView.cellOrigin(c, px, py);
            TileArt::flag(tft, px, py, DEMO_TILE, demoView.parity(c), 1);
            raised = c;
            raising = true;
            return;
        }
        Field::Result r = demo.open(c);
        demoView.beginReveal();
        if (r == Field::WON)
        {
            phase = Demo::SWEEPING;
            step = 0;
        }
        return;
    }
}

void TitleScreen::update(unsigned long now)
{
    if (demoView.revealing())
    {
        if (now - lastDemo >= REVEAL_MS)
        {
            lastDemo = now;
            demoView.revealStep();
        }
        return;
    }
    switch (phase)
    {
    case Demo::WAITING:
        if (now - lastDemo >= WAIT_MS)
        {
            lastDemo = now;
            demo.open({(uint8_t)(random() % DEMO_COLS), (uint8_t)(random() % DEMO_ROWS)});
            demoView.beginReveal();
            phase = Demo::SOLVING;
        }
        break;
    case Demo::SOLVING:
        if (now - lastDemo >= SOLVE_MS)
        {
            lastDemo = now;
            if (raising)
            {
                demoView.draw(raised);
                raising = false;
            }
            advanceDemo();
        }
        break;
    case Demo::SWEEPING:
        if (now - lastDemo >= SWEEP_MS)
        {
            lastDemo = now;
            demoView.sweep(step++);
            if (step >= demoView.diagonals() + TileArt::FLAG_FRAMES)
            {
                phase = Demo::RESTING;
            }
        }
        break;
    case Demo::RESTING:
        if (now - lastDemo >= REST_MS)
        {
            lastDemo = now;
            phase = Demo::COVERING;
            step = 0;
        }
        break;
    case Demo::COVERING:
        if (now - lastDemo >= COVER_MS)
        {
            lastDemo = now;
            demoView.cover(step++);
            if (step >= demoView.diagonals())
            {
                restartDemo(now);
            }
        }
        break;
    }
}

void TitleScreen::onUpPress()
{
    selected = (selected + ROW_COUNT - 1) % ROW_COUNT;
    refresh();
    sound.playMenu();
}

void TitleScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    refresh();
    sound.playMenu();
}

void TitleScreen::onConfirmPress()
{
    sound.playMenu();
    if (selected == 0)
    {
        screenManager.openModal(&playScreen);
        return;
    }
    if (selected == 1)
    {
        records.level = (records.level + 1) % LEVEL_COUNT;
        refresh();
        save();
        return;
    }
    settings.theme = (settings.theme + 1) % THEME_COUNT;
    settingsChanged();
    paint(tft);
}
