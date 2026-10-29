#include "TitleScreen.h"
#include "../ui/CellArt.h"
#include "../ui/Themes.h"

static const int16_t TRACK_X = 10;
static const int16_t TRACK_Y = 36;

static const int16_t ROW_X = 5;
static const int16_t PLAY_Y = 146;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t BEST_Y = 250;
static const int16_t BEST_PITCH = 12;

static const unsigned long CRAWL_MS = 150;

// 5x7, a row per byte with the leftmost dot in bit 4
static const uint8_t LOGO[][7] = {
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
    {0x11, 0x19, 0x15, 0x15, 0x13, 0x11, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
};
static const uint8_t LOGO_LETTERS = sizeof(LOGO) / sizeof(LOGO[0]);
static const int16_t DOT = 6;

TitleScreen::TitleScreen(TFT_eSPI &tft, Records &records, void (*save)(), Settings &settings,
                         void (*settingsChanged)(), ScreenManager &screenManager, SoundManager &sound,
                         Screen &playScreen)
    : tft(tft),
      records(records),
      save(save),
      settings(settings),
      settingsChanged(settingsChanged),
      screenManager(screenManager),
      sound(sound),
      playScreen(playScreen),
      header("Snake"),
      bestTitle(120, BEST_Y, "BEST SCORES", Theme::TEXT_2, 1, TC_DATUM)
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
        bestNames[i] = Text(60, y, LEVELS[i].name, Theme::TEXT_2, 1, TL_DATUM);
        bestScores[i] = Text(180, y, "", Theme::TEXT_2, 1, TR_DATUM);
        addElement(&bestNames[i]);
        addElement(&bestScores[i]);
    }
    memset(fat, 0, sizeof(fat));
    crawlerHead = CRAWLER_LENGTH;
    foodAt = TRACK_COLS + 2;
}

void TitleScreen::refresh()
{
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    rows[1].setValue(LEVELS[records.level].name);
    rows[2].setValue(THEMES[Theme::getActive()].name);
    char buf[8];
    for (uint8_t i = 0; i < LEVEL_COUNT; i++)
    {
        snprintf(buf, sizeof(buf), "%04u", (unsigned)records.best[i]);
        bestScores[i].setText(buf);
        Color c = i == records.level ? Theme::TEXT : Theme::TEXT_DIM;
        bestNames[i].setColor(c);
        bestScores[i].setColor(c);
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
    drawLogo();
    for (uint8_t i = 0; i < TRACK_LENGTH; i++)
    {
        drawTrackCell(i);
    }
}

// Clockwise from the top left corner
Cell TitleScreen::trackCell(uint8_t i) const
{
    const uint8_t right = TRACK_COLS - 1, bottom = TRACK_ROWS - 1;
    if (i < right)
    {
        return {i, 0};
    }
    i -= right;
    if (i < bottom)
    {
        return {right, i};
    }
    i -= bottom;
    if (i < right)
    {
        return {(uint8_t)(right - i), bottom};
    }
    i -= right;
    return {0, (uint8_t)(bottom - i)};
}

Dir TitleScreen::trackEntered(uint8_t i) const
{
    Cell from = trackCell(back(i, 1)), to = trackCell(i);
    if (to.x > from.x)
    {
        return Dir::RIGHT;
    }
    if (to.x < from.x)
    {
        return Dir::LEFT;
    }
    return to.y > from.y ? Dir::DOWN : Dir::UP;
}

bool TitleScreen::onCrawler(uint8_t i) const
{
    return (uint8_t)((crawlerHead + TRACK_LENGTH - i) % TRACK_LENGTH) < CRAWLER_LENGTH;
}

void TitleScreen::placeFood()
{
    do
    {
        foodAt = (uint8_t)random(TRACK_LENGTH);
    } while (onCrawler(foodAt) || foodAt == (crawlerHead + 1) % TRACK_LENGTH);
}

void TitleScreen::drawLogo()
{
    const int16_t left = TRACK_X + CellArt::SIZE, top = TRACK_Y + CellArt::SIZE;
    const int16_t w = (TRACK_COLS - 2) * CellArt::SIZE, h = (TRACK_ROWS - 2) * CellArt::SIZE;
    tft.fillRect(left, top, w, h, Theme::FIELD);
    const int16_t dotCols = w / DOT, dotRows = h / DOT;
    const int16_t ox = left + (w - dotCols * DOT) / 2, oy = top + (h - dotRows * DOT) / 2;
    const int16_t logoW = LOGO_LETTERS * 6 - 1;
    const int16_t firstCol = (dotCols - logoW) / 2, firstRow = (dotRows - 7) / 2;
    uint16_t ghost = Theme::GHOST, ink = Theme::INK;
    for (int16_t r = 0; r < dotRows; r++)
    {
        for (int16_t c = 0; c < dotCols; c++)
        {
            int16_t lc = c - firstCol, lr = r - firstRow;
            bool lit = false;
            if (lc >= 0 && lc < logoW && lr >= 0 && lr < 7 && lc % 6 != 5)
            {
                lit = (LOGO[lc / 6][lr] >> (4 - lc % 6)) & 1;
            }
            tft.fillRect(ox + c * DOT, oy + r * DOT, DOT - 1, DOT - 1, lit ? ink : ghost);
        }
    }
}

void TitleScreen::drawTrackCell(uint8_t i)
{
    Cell c = trackCell(i);
    int16_t px = TRACK_X + c.x * CellArt::SIZE, py = TRACK_Y + c.y * CellArt::SIZE;
    uint8_t along = (uint8_t)((crawlerHead + TRACK_LENGTH - i) % TRACK_LENGTH);
    if (along == 0)
    {
        bool mouthOpen = foodAt == (crawlerHead + 1) % TRACK_LENGTH;
        CellArt::head(tft, px, py, trackEntered(i), mouthOpen, eyesShut);
    }
    else if (along == CRAWLER_LENGTH - 1)
    {
        CellArt::tail(tft, px, py, trackEntered((i + 1) % TRACK_LENGTH));
    }
    else if (along < CRAWLER_LENGTH)
    {
        uint8_t joins = CellArt::joinBit(trackEntered((i + 1) % TRACK_LENGTH)) | CellArt::joinBit(opposite(trackEntered(i)));
        CellArt::body(tft, px, py, joins, fat[i], ((c.x + c.y) & 1) != 0);
    }
    else if (i == foodAt)
    {
        CellArt::food(tft, px, py, true);
    }
    else
    {
        CellArt::empty(tft, px, py);
    }
}

void TitleScreen::update(unsigned long now)
{
    if ((long)(now - nextBlink) >= 0)
    {
        eyesShut = !eyesShut;
        nextBlink = now + (eyesShut ? 140 : 1500 + random(2500));
        drawTrackCell(crawlerHead);
    }
    if (now - lastCrawl < CRAWL_MS)
    {
        return;
    }
    lastCrawl = now;

    uint8_t oldTail = back(crawlerHead, CRAWLER_LENGTH - 1);
    crawlerHead = (crawlerHead + 1) % TRACK_LENGTH;
    fat[oldTail] = false;
    bool ate = crawlerHead == foodAt;
    if (ate)
    {
        fat[crawlerHead] = true;
        placeFood();
        drawTrackCell(foodAt);
    }
    drawTrackCell(oldTail);
    drawTrackCell(back(crawlerHead, CRAWLER_LENGTH - 1));
    drawTrackCell(back(crawlerHead, 1));
    drawTrackCell(crawlerHead);
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
