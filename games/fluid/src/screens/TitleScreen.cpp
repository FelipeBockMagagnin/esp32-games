#include "TitleScreen.h"
#include <math.h>
#include "../fluid/Palettes.h"
#include "../ui/Themes.h"

static const int16_t BANNER_X = 5;
static const int16_t BANNER_Y = 34;
static const int16_t BANNER_W = 230;
static const int16_t BANNER_H = 108;

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 150;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t HELP_Y = 290;

static const unsigned long WAVE_MS = 90;
static const uint8_t WAVE_STEPS = 32;
// The name sits above the waves' reach, so they roll without touching it
static const int16_t WAVES_TOP = 46;

enum Row : uint8_t
{
    ROW_INK,
    ROW_WATER,
    ROW_PALETTE,
    ROW_THEME
};

TitleScreen::TitleScreen(TFT_eSPI &tft, Prefs &prefs, void (*save)(), Settings &settings,
                         void (*settingsChanged)(), ScreenManager &screenManager, SoundManager &sound,
                         Screen &playScreen)
    : tft(tft),
      prefs(prefs),
      save(save),
      settings(settings),
      settingsChanged(settingsChanged),
      screenManager(screenManager),
      sound(sound),
      playScreen(playScreen),
      header("Fluid"),
      help(120, HELP_Y, "Arrows stir the ink or tilt the water", Theme::TEXT_2, 1, TC_DATUM)
{
    addElement(&header);
    setHeader(&header);
    static const char *const TITLES[ROW_COUNT] = {"Ink", "Water", "Palette", "Theme"};
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        rows[i].setTitle(TITLES[i]);
        addElement(&rows[i]);
    }
    addElement(&help);
}

void TitleScreen::refresh()
{
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    rows[ROW_INK].setValue(prefs.mode == (uint8_t)Mode::INK ? "Last" : "");
    rows[ROW_WATER].setValue(prefs.mode == (uint8_t)Mode::WATER ? "Last" : "");
    rows[ROW_PALETTE].setValue(PALETTES[prefs.palette].name);
    rows[ROW_THEME].setValue(THEMES[Theme::getActive()].name);
}

void TitleScreen::onEnter(TFT_eSPI &display)
{
    selected = prefs.mode == (uint8_t)Mode::WATER ? ROW_WATER : ROW_INK;
    paint(display);
}

void TitleScreen::paint(TFT_eSPI &display)
{
    refresh();
    Screen::onEnter(display);
    drawBanner(true);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_DIM);
    tft.drawString("OK opens the menu", 120, HELP_Y + 12);
}

// Four bands of ink, one per stop of the palette, rolling as waves under the name
void TitleScreen::drawBanner(bool whole)
{
    const Palette &p = PALETTES[prefs.palette];
    if (whole)
    {
        tft.fillRect(BANNER_X, BANNER_Y, BANNER_W, WAVES_TOP, rgb565(p.background));
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(4);
        tft.setTextColor(rgb565(0xffffff));
        tft.drawString("FLUID", 120, BANNER_Y + 8);
    }
    const float phase = wavePhase * (6.2831853f / WAVE_STEPS);
    tft.startWrite();
    for (int16_t x = 0; x < BANNER_W; x++)
    {
        int16_t edges[5];
        for (uint8_t k = 0; k < 4; k++)
        {
            float base = 58 + k * 14;
            float wave = 7 * sinf(x * 0.045f + phase + k * 1.3f) + 3 * sinf(x * 0.11f - phase * 2 + k);
            edges[k + 1] = (int16_t)(base + wave);
        }
        tft.drawFastVLine(BANNER_X + x, BANNER_Y + WAVES_TOP, edges[1] - WAVES_TOP, rgb565(p.background));
        int16_t top = edges[1];
        for (uint8_t k = 0; k < 4; k++)
        {
            int16_t bottom = k < 3 && edges[k + 2] < BANNER_H ? edges[k + 2] : BANNER_H;
            if (bottom > top)
            {
                tft.drawFastVLine(BANNER_X + x, BANNER_Y + top, bottom - top, rgb565(p.ink[k]));
                top = bottom;
            }
        }
    }
    tft.endWrite();
    tft.drawRect(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, Theme::FRAME_DIM);
}

void TitleScreen::update(unsigned long now)
{
    if (now - lastWave < WAVE_MS)
    {
        return;
    }
    lastWave = now;
    wavePhase = (uint8_t)((wavePhase + 1) % WAVE_STEPS);
    drawBanner(false);
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
    switch (selected)
    {
    case ROW_INK:
    case ROW_WATER:
        prefs.mode = selected == ROW_INK ? (uint8_t)Mode::INK : (uint8_t)Mode::WATER;
        save();
        screenManager.openModal(&playScreen);
        return;
    case ROW_PALETTE:
        prefs.palette = (uint8_t)((prefs.palette + 1) % PALETTE_COUNT);
        save();
        refresh();
        drawBanner(true);
        return;
    default:
        settings.theme = (uint8_t)((settings.theme + 1) % THEME_COUNT);
        settingsChanged();
        paint(tft);
    }
}
