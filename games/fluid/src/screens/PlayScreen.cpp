#include "PlayScreen.h"
#include <math.h>
#include <stdlib.h>
#include "../fluid/Palettes.h"

static const unsigned long FRAME_MS = 33;
static const int CELL = FieldRender::FIELD_W / InkSim::W;

// The nozzle, in cells per frame: how fast a held arrow speeds it up, its top speed, and how
// much of its speed it keeps each frame once let go
static const float NOZZLE_ACCEL = 0.22f;
static const float NOZZLE_MAX = 1.3f;
static const float NOZZLE_COAST = 0.8f;
static const float NOZZLE_PUSH = 1.6f;
static const float NOZZLE_RADIUS = 2.6f;
static const float INK_CYCLE = 0.0025f; // Of the palette's loop per frame of ink laid

static const int WATER_STEPS = 2; // Per frame
static const float GRAVITY = 0.15f;
static const float SHAKE = 6.0f;

static const int16_t BOX_X = 30;
static const int16_t BOX_Y = 84;
static const int16_t BOX_W = 180;
static const int16_t BOX_PITCH = 22;
static const int16_t BOX_H = 44 + 5 * BOX_PITCH;

PlayScreen::PlayScreen(TFT_eSPI &tft, Prefs &prefs, void (*save)(), ScreenManager &screenManager,
                       SoundManager &sound)
    : tft(tft), prefs(prefs), save(save), screenManager(screenManager), sound(sound)
{
}

void PlayScreen::onEnter(TFT_eSPI &display)
{
    Screen::onEnter(display);
    choice = 0;
    held = 0;
    render.setPalette(PALETTES[prefs.palette]);
    if (!start())
    {
        stop();
        state = State::NO_MEMORY;
        drawNoMemory();
        return;
    }
    state = State::RUNNING;
    reset();
    lastFrame = millis();
    sound.playStart();
    paint();
}

void PlayScreen::onExit()
{
    stop();
    Screen::onExit();
}

bool PlayScreen::start()
{
    for (uint16_t *&band : bands)
    {
        if (band == nullptr)
        {
            band = (uint16_t *)malloc(FieldRender::FIELD_W * BAND_ROWS * sizeof(uint16_t));
        }
        if (band == nullptr)
        {
            return false;
        }
    }
    if (mode() == Mode::INK)
    {
        return ink.begin();
    }
    return water.begin() && render.beginWater();
}

void PlayScreen::stop()
{
    ink.end();
    water.end();
    render.endWater();
    for (uint16_t *&band : bands)
    {
        free(band);
        band = nullptr;
    }
}

void PlayScreen::reset()
{
    if (mode() == Mode::INK)
    {
        ink.clear();
        nozzleX = InkSim::W * 0.5f;
        nozzleY = InkSim::H * 0.5f;
        nozzleVX = nozzleVY = 0;
        return;
    }
    water.fill();
    fallX = 0;
    fallY = 1;
    water.setGravity(0, GRAVITY);
}

void PlayScreen::moveNozzle()
{
    float ax = ((held & HELD_RIGHT) ? 1.0f : 0.0f) - ((held & HELD_LEFT) ? 1.0f : 0.0f);
    float ay = ((held & HELD_DOWN) ? 1.0f : 0.0f) - ((held & HELD_UP) ? 1.0f : 0.0f);
    if (ax == 0 && ay == 0)
    {
        nozzleVX *= NOZZLE_COAST;
        nozzleVY *= NOZZLE_COAST;
    }
    else
    {
        nozzleVX += ax * NOZZLE_ACCEL;
        nozzleVY += ay * NOZZLE_ACCEL;
        float s = sqrtf(nozzleVX * nozzleVX + nozzleVY * nozzleVY);
        if (s > NOZZLE_MAX)
        {
            nozzleVX *= NOZZLE_MAX / s;
            nozzleVY *= NOZZLE_MAX / s;
        }
    }
    nozzleX += nozzleVX;
    nozzleY += nozzleVY;
    const float margin = 1.5f;
    if (nozzleX < margin || nozzleX > InkSim::W - margin)
    {
        nozzleX = nozzleX < margin ? margin : InkSim::W - margin;
        nozzleVX = -nozzleVX * 0.5f;
    }
    if (nozzleY < margin || nozzleY > InkSim::H - margin)
    {
        nozzleY = nozzleY < margin ? margin : InkSim::H - margin;
        nozzleVY = -nozzleVY * 0.5f;
    }
    if (held & (HELD_UP | HELD_DOWN | HELD_LEFT | HELD_RIGHT))
    {
        inkPhase += INK_CYCLE;
        ink.stir(nozzleX, nozzleY, nozzleVX * NOZZLE_PUSH, nozzleVY * NOZZLE_PUSH,
                 inkAt(PALETTES[prefs.palette], inkPhase), NOZZLE_RADIUS);
    }
}

void PlayScreen::frame()
{
    frames++;
    if (mode() == Mode::INK)
    {
        moveNozzle();
        ink.step();
        return;
    }
    for (int i = 0; i < WATER_STEPS; i++)
    {
        water.step();
    }
}

// The nozzle's ring, in the ink it lays, round a dark edge so it shows on any dye
void PlayScreen::drawNozzle(uint16_t *band, int top)
{
    Rgb c = inkAt(PALETTES[prefs.palette], inkPhase);
    uint16_t lit = (uint16_t)((int)(c.r * 31) << 11 | (int)(c.g * 63) << 5 | (int)(c.b * 31));
    lit = (uint16_t)(lit << 8 | lit >> 8);
    int cx = (int)(nozzleX * CELL), cy = (int)(nozzleY * CELL);
    for (int y = cy - 8; y <= cy + 8; y++)
    {
        if (y < top || y >= top + BAND_ROWS)
        {
            continue;
        }
        for (int x = cx - 8; x <= cx + 8; x++)
        {
            if (x < 0 || x >= FieldRender::FIELD_W)
            {
                continue;
            }
            int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            uint16_t *p = band + (y - top) * FieldRender::FIELD_W + x;
            if (d2 >= 20 && d2 <= 36)
            {
                *p = lit;
            }
            else if (d2 > 36 && d2 <= 56)
            {
                *p = 0;
            }
        }
    }
}

void PlayScreen::paint()
{
    if (mode() == Mode::WATER)
    {
        render.splat(water);
    }
    uint8_t which = 0;
    tft.startWrite();
    for (int top = 0; top < FieldRender::FIELD_H; top += BAND_ROWS)
    {
        uint16_t *band = bands[which];
        if (mode() == Mode::INK)
        {
            render.inkRows(ink, top, BAND_ROWS, band);
            drawNozzle(band, top);
        }
        else
        {
            render.waterRows(top, BAND_ROWS, band);
        }
        if (dma)
        {
            tft.pushImageDMA(0, top, FieldRender::FIELD_W, BAND_ROWS, band);
            which ^= 1;
        }
        else
        {
            tft.pushImage(0, top, FieldRender::FIELD_W, BAND_ROWS, band);
        }
    }
    if (dma)
    {
        tft.dmaWait();
    }
    tft.endWrite();
}

void PlayScreen::update(unsigned long now)
{
    if (state != State::RUNNING || now - lastFrame < FRAME_MS)
    {
        return;
    }
    lastFrame = now - lastFrame < 2 * FRAME_MS ? lastFrame + FRAME_MS : now;
#ifdef DEV_MODE
    unsigned long began = micros();
    frame();
    unsigned long simulated = micros();
    paint();
    static unsigned long simUs = 0, paintUs = 0, since = 0;
    static uint16_t count = 0;
    simUs += simulated - began;
    paintUs += micros() - simulated;
    if (++count == 1 || now - since >= 5000)
    {
        Serial.printf("%u frames in %lu ms: sim %lu us, paint %lu us a frame, heap %u\n", count, now - since,
                      simUs / count, paintUs / count, (unsigned)ESP.getFreeHeap());
        simUs = paintUs = 0;
        count = 0;
        since = now;
    }
#else
    frame();
    paint();
#endif
}

void PlayScreen::drawMenu()
{
    uint16_t card = Theme::CARD, frame = Theme::FRAME, text = Theme::TEXT, dim = Theme::TEXT_2;
    tft.fillRect(BOX_X, BOX_Y, BOX_W, BOX_H, card);
    tft.drawRect(BOX_X, BOX_Y, BOX_W, BOX_H, frame);
    tft.drawRect(BOX_X + 2, BOX_Y + 2, BOX_W - 4, BOX_H - 4, frame);

    int16_t cx = BOX_X + BOX_W / 2;
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(text);
    tft.drawString(mode() == Mode::INK ? "INK" : "WATER", cx, BOX_Y + 12);

    const bool inkMode = mode() == Mode::INK;
    const char *labels[OPTION_COUNT] = {"Resume", inkMode ? "Clear" : "Refill", inkMode ? "Water" : "Ink",
                                        PALETTES[prefs.palette].name, "Quit"};
    for (uint8_t i = 0; i < OPTION_COUNT; i++)
    {
        int16_t y = BOX_Y + 38 + i * BOX_PITCH;
        bool on = i == choice;
        if (on)
        {
            tft.fillRect(BOX_X + 8, y - 3, BOX_W - 16, BOX_PITCH - 2, (uint16_t)Theme::ROW_SELECTED);
            tft.fillTriangle(BOX_X + 14, y, BOX_X + 14, y + 13, BOX_X + 21, y + 6, (uint16_t)Theme::SELECT);
        }
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(on ? text : dim);
        tft.drawString(labels[i], BOX_X + 30, y);
        if (i == PALETTE)
        {
            // The palette's four inks, as a strip after its name
            for (uint8_t k = 0; k < 4; k++)
            {
                tft.fillRect(BOX_X + BOX_W - 50 + k * 10, y + 2, 8, 10, rgb565(PALETTES[prefs.palette].ink[k]));
            }
        }
    }
}

void PlayScreen::drawNoMemory()
{
    tft.fillScreen(Theme::BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(Theme::BAD);
    tft.drawString("Out of memory", 120, 150);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString("OK goes back", 120, 176);
}

void PlayScreen::tilt(int8_t dx, int8_t dy, uint8_t dir)
{
    if (dx == fallX && dy == fallY)
    {
        uint32_t seed = frames * 2654435761u | 1;
        water.shake(-dx * SHAKE, -dy * SHAKE, seed);
        sound.playShake();
        return;
    }
    fallX = dx;
    fallY = dy;
    water.setGravity(dx * GRAVITY, dy * GRAVITY);
    sound.playTilt(dir);
}

void PlayScreen::arrow(int8_t dx, int8_t dy, uint8_t dir)
{
    if (state == State::MENU)
    {
        if (dy != 0)
        {
            choice = (uint8_t)((choice + OPTION_COUNT + dy) % OPTION_COUNT);
            sound.playMenu();
            drawMenu();
        }
        else if (choice == PALETTE)
        {
            prefs.palette = (uint8_t)((prefs.palette + PALETTE_COUNT + dx) % PALETTE_COUNT);
            save();
            render.setPalette(PALETTES[prefs.palette]);
            sound.playMenu();
            paint();
            drawMenu();
        }
        return;
    }
    if (state == State::RUNNING && mode() == Mode::WATER)
    {
        tilt(dx, dy, dir);
    }
}

void PlayScreen::onUpPress()
{
    arrow(0, -1, 0);
}

void PlayScreen::onDownPress()
{
    arrow(0, 1, 1);
}

bool PlayScreen::onBackPress()
{
    arrow(-1, 0, 2);
    return true;
}

bool PlayScreen::onRightPress()
{
    arrow(1, 0, 3);
    return true;
}

void PlayScreen::onConfirmPress()
{
    if (state == State::NO_MEMORY)
    {
        screenManager.closeModal();
        return;
    }
    if (state == State::RUNNING)
    {
        state = State::MENU;
        choice = RESUME;
        sound.playPause();
        drawMenu();
        return;
    }
    switch (choice)
    {
    case RESUME:
        sound.playMenu();
        state = State::RUNNING;
        lastFrame = millis();
        paint();
        break;
    case CLEAR:
        sound.playClear();
        reset();
        state = State::RUNNING;
        lastFrame = millis();
        paint();
        break;
    case SWITCH:
        sound.playMenu();
        stop();
        prefs.mode = mode() == Mode::INK ? (uint8_t)Mode::WATER : (uint8_t)Mode::INK;
        save();
        onEnter(tft);
        break;
    case PALETTE:
        arrow(1, 0, 3);
        break;
    default:
        sound.playMenu();
        screenManager.closeModal();
        break;
    }
}
