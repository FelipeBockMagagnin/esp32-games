#include "TitleScreen.h"
#include "../ui/BlockArt.h"
#include "../ui/Themes.h"

static const int16_t LOGO_TILE = 8;
static const int16_t LOGO_Y = 50;
static const uint8_t LOGO_ROWS = 5;
static const int16_t LOGO_LIFT = 16;

static const int16_t ROW_X = 5;
static const int16_t PLAY_Y = 102;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t BEST_Y = 210;
static const int16_t BEST_PITCH = 13;
static const int16_t CONTROLS_Y = 274;

static const unsigned long LOGO_FRAME_MS = 36;
static const uint8_t LETTER_STAGGER = 3;
static const uint8_t REPLAY_FRAMES = 220;

struct Letter
{
    uint8_t width;
    uint8_t rows[LOGO_ROWS]; // Bit 0 is the leftmost column
};

static const Letter LETTERS[] = {
    {3, {0b111, 0b001, 0b111, 0b001, 0b111}},           // E
    {5, {0b10001, 0b11011, 0b10101, 0b10001, 0b10001}}, // M
    {3, {0b111, 0b101, 0b111, 0b001, 0b001}},           // P
    {1, {0b1, 0b1, 0b1, 0b1, 0b1}},                     // I
    {3, {0b001, 0b001, 0b001, 0b001, 0b111}},           // L
    {3, {0b101, 0b101, 0b111, 0b101, 0b101}},           // H
    {3, {0b111, 0b101, 0b111, 0b101, 0b101}},           // A
};

// From high up, with a small bounce on landing
static const int8_t DROP_LIFT[] = {LOGO_LIFT, 11, 6, 0, 3, 0};
static const uint8_t DROP_FRAMES = sizeof(DROP_LIFT);

static int16_t letterX(uint8_t letter)
{
    int16_t columns = 0;
    for (uint8_t i = 0; i < letter; i++)
    {
        columns += LETTERS[i].width + 1;
    }
    return (240 - 27 * LOGO_TILE) / 2 + columns * LOGO_TILE;
}

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
      header("Empilha"),
      bestTitle(120, BEST_Y, "BEST RUNS", Theme::TEXT_2, 1, TC_DATUM)
{
    addElement(&header);
    setHeader(&header);
    static const char *const TITLES[ROW_COUNT] = {"Play", "Mode", "Theme"};
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, PLAY_Y + i * ROW_PITCH);
        rows[i].setTitle(TITLES[i]);
        addElement(&rows[i]);
    }
    addElement(&bestTitle);
    for (uint8_t i = 0; i < MODE_COUNT; i++)
    {
        int16_t y = BEST_Y + 14 + i * BEST_PITCH;
        bestNames[i] = Text(30, y, MODE_NAMES[i], Theme::TEXT_2, 1, TL_DATUM);
        bestValues[i] = Text(150, y, "", Theme::TEXT_2, 1, TR_DATUM);
        bestPlays[i] = Text(212, y, "", Theme::TEXT_2, 1, TR_DATUM);
        addElement(&bestNames[i]);
        addElement(&bestValues[i]);
        addElement(&bestPlays[i]);
    }
    controlLines[0] = Text(120, CONTROLS_Y, "Arrows move, Down drops", Theme::TEXT_DIM, 1, TC_DATUM);
    controlLines[1] = Text(120, CONTROLS_Y + 12, "OK turns, Up slams", Theme::TEXT_DIM, 1, TC_DATUM);
    addElement(&controlLines[0]);
    addElement(&controlLines[1]);
}

void TitleScreen::refresh()
{
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    rows[1].setValue(MODE_NAMES[records.mode]);
    rows[2].setValue(THEMES[Theme::getActive()].name);
    char buf[16];
    for (uint8_t i = 0; i < MODE_COUNT; i++)
    {
        if ((Mode)i == Mode::SPRINT)
        {
            formatTime(records.best[i], buf, sizeof(buf));
        }
        else
        {
            snprintf(buf, sizeof(buf), "%u", (unsigned)records.best[i]);
        }
        bestValues[i].setText(buf);
        snprintf(buf, sizeof(buf), "%u runs", (unsigned)records.plays[i]);
        bestPlays[i].setText(buf);
        Color c = i == records.mode ? Theme::TEXT : Theme::TEXT_DIM;
        bestNames[i].setColor(c);
        bestValues[i].setColor(c);
        bestPlays[i].setColor(c);
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
    logoFrame = 0;
    lastLogo = millis();
    for (uint8_t i = 0; i < LETTER_COUNT; i++)
    {
        logoShown[i] = -1;
    }
}

void TitleScreen::drawLetter(uint8_t letter, int16_t lift)
{
    const Letter &glyph = LETTERS[letter];
    int16_t x = letterX(letter);
    tft.fillRect(x, LOGO_Y - LOGO_LIFT, glyph.width * LOGO_TILE, LOGO_ROWS * LOGO_TILE + LOGO_LIFT, Theme::BG);
    for (uint8_t row = 0; row < LOGO_ROWS; row++)
    {
        for (uint8_t col = 0; col < glyph.width; col++)
        {
            if (glyph.rows[row] & (1 << col))
            {
                BlockArt::block(tft, x + col * LOGO_TILE, LOGO_Y - lift + row * LOGO_TILE, LOGO_TILE, letter, Theme::BG);
            }
        }
    }
}

void TitleScreen::update(unsigned long now)
{
    if (now - lastLogo < LOGO_FRAME_MS)
    {
        return;
    }
    lastLogo = now;
    for (uint8_t i = 0; i < LETTER_COUNT; i++)
    {
        int16_t frame = logoFrame - i * LETTER_STAGGER;
        if (frame < 0)
        {
            continue;
        }
        int8_t lift = frame < DROP_FRAMES ? DROP_LIFT[frame] : 0;
        if (lift != logoShown[i])
        {
            logoShown[i] = lift;
            drawLetter(i, lift);
        }
    }
    if (++logoFrame >= REPLAY_FRAMES)
    {
        logoFrame = 0;
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
        records.mode = (records.mode + 1) % MODE_COUNT;
        refresh();
        save();
        return;
    }
    settings.theme = (settings.theme + 1) % THEME_COUNT;
    settingsChanged();
    paint(tft);
}
