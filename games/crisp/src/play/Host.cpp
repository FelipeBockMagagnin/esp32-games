#include "Host.h"
#include <float.h>
#include "Crisp.h"
#include "Launcher.h"
#include "UI.h"
#include "machineDependent.h"
#include "../ui/Theme.h"
#ifndef CRISP_PC
#include <esp_timer.h>
#endif

static Host *host = nullptr;

#ifndef CRISP_PC
// Every game's zero-initialised statics, laid over each other (tools/carts.py, overlay.ld)
extern "C" uint8_t crisp_overlay_start[], crisp_overlay_end[];
#endif

// The game starts as from a fresh boot: its statics zero, whatever the last game left there
static void launch(uint16_t game)
{
#ifndef CRISP_PC
    memset(crisp_overlay_start, 0, crisp_overlay_end - crisp_overlay_start);
#endif
    crispStart(game);
}

extern "C" {
void md_drawRect(float x, float y, float w, float h, unsigned char r, unsigned char g, unsigned char b)
{
    host->drawRect(x, y, w, h, r, g, b);
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3], float x, float y, int)
{
    host->drawCharacter(grid, x, y);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b)
{
    host->clearView(r, g, b);
}

// The library clears the whole panel when a game starts; the host draws what surrounds the game
void md_clearScreen(unsigned char, unsigned char, unsigned char) {}

void md_playTone(float freq, float duration, float when)
{
    host->playTone(freq, duration, when);
}

void md_stopTone()
{
    host->stopTone();
}

float md_getAudioTime()
{
    return host->audioTime();
}

void md_initView(int w, int h)
{
    host->initView(w, h);
}

void md_consoleLog(char *msg)
{
#ifdef CRISP_PC
    fprintf(stderr, "cglp: %s\n", msg);
#else
    Serial.println(msg);
#endif
}
}

static const uint32_t FRAME_US = 1000000 / 60;
// Behind by this many frames, the game stops catching up rather than run on unseen
static const uint8_t MAX_LATE = 4;
// Pixels a frame the pointer moves, from a nudge to a sweep across the screen once held
static const float POINTER_SLOW = 0.8f;
static const float POINTER_FAST = 2.4f;
static const uint8_t POINTER_RAMP = 30;

static const char *const MENU_NAMES[] = {"Resume", "Restart game", "Quit to list", "Game menu"};
static const char *const KEY_NAMES[] = {"Up", "Down", "Left", "Right"};

#ifndef CRISP_PC
static portMUX_TYPE toneLock = portMUX_INITIALIZER_UNLOCKED;
#define TONE_LOCK() portENTER_CRITICAL(&toneLock)
#define TONE_UNLOCK() portEXIT_CRITICAL(&toneLock)
#else
#define TONE_LOCK()
#define TONE_UNLOCK()
#endif

// The panel takes RGB565 high byte first, and the band goes out as it sits in memory
static uint16_t panelOrder(uint16_t c)
{
    return (uint16_t)(c << 8 | c >> 8);
}

static uint32_t fnv(uint32_t h, const uint8_t *p, int n)
{
    for (int i = 0; i < n; i++)
    {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

Host::Host(TFT_eSPI &tft, Pins pins, SoundManager &sound, void (*quit)(uint16_t index),
           void (*scored)(uint16_t index))
    : tft(tft), pins(pins), sound(sound), quit(quit), scored(scored)
{
    host = this;
    for (Tone &t : tones)
    {
        t.when = FLT_MAX;
    }
}

bool Host::begin()
{
    // From the heap, which has room the ESP32's static RAM has not; the bands must be DMA-capable
    frame = (uint8_t *)malloc(FRAME_BYTES);
    for (uint16_t *&b : band)
    {
#ifdef CRISP_PC
        b = (uint16_t *)malloc(240 * BAND_ROWS * sizeof(uint16_t));
#else
        b = (uint16_t *)heap_caps_malloc(240 * BAND_ROWS * sizeof(uint16_t), MALLOC_CAP_DMA);
#endif
    }
    dma = tft.initDMA();
#ifndef CRISP_PC
    esp_timer_create_args_t args = {};
    args.callback = [](void *) { host->soundTick(); };
    args.name = "crispTone";
    esp_timer_handle_t timer;
    if (esp_timer_create(&args, &timer) == ESP_OK)
    {
        esp_timer_start_periodic(timer, 2000);
    }
#endif
    return dma;
}

void Host::start(uint16_t i)
{
    index = i;
    cart = &CARTS[i];
    launcherShown = launcherInstalled();
    // The Confirm press that started the game must not reach it
    waitRelease = true;
    audioStartUs = micros();
    tft.fillScreen(Theme::BG);
    launch(i);
    shownHi = crispHiScore(i);
    pointerX = viewW / 2.0f;
    pointerY = viewH / 2.0f;
    drawStrip();
    active = true;
    nextFrameUs = micros();
    lateFrames = 0;
    statSince = millis();
    statFrames = statShown = 0;
}

void Host::initView(int w, int h)
{
    if (w < 1 || w > 240)
    {
        w = 100;
    }
    if (h < 1 || w * h > FRAME_BYTES)
    {
        h = FRAME_BYTES / w;
    }
    viewW = w;
    viewH = h;
    paletteCount = 0;
    lastRgb = 0xFFFFFFFF;

    int scale = DISPLAY_W / w;
    if (scale > 2)
    {
        scale = 2;
    }
    while (scale > 1 && h * scale > DISPLAY_H - 20)
    {
        scale--;
    }
    if (scale >= 2)
    {
        outW = w * scale;
        outH = h * scale;
    }
    else
    {
        outW = DISPLAY_W;
        outH = h * DISPLAY_W / w;
    }
    gameX = (DISPLAY_W - outW) / 2;
    gameY = outH + 8 + 40 <= DISPLAY_H ? 8 : 0;
    stripY = gameY + outH + (gameY > 0 ? 8 : 0);
    for (int x = 0; x < outW; x++)
    {
        xMap[x] = (uint8_t)(x * w / outW);
    }
    for (int y = 0; y < outH; y++)
    {
        yMap[y] = (uint8_t)(y * h / outH);
    }
    memset(frame, 0, FRAME_BYTES);
    repaintAll = true;
}

uint8_t Host::colorIndex(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t rgb = (uint32_t)r << 16 | g << 8 | b;
    if (rgb == lastRgb)
    {
        return lastIndex;
    }
    uint8_t i = 0;
    while (i < paletteCount && paletteRgb[i] != rgb)
    {
        i++;
    }
    if (i == paletteCount)
    {
        if (paletteCount == PALETTE_SIZE)
        {
            return 0;
        }
        paletteRgb[i] = rgb;
        palette[i] = panelOrder((uint16_t)((r & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3));
        paletteCount++;
    }
    lastRgb = rgb;
    lastIndex = i;
    return i;
}

void Host::clearView(uint8_t r, uint8_t g, uint8_t b)
{
    memset(frame, colorIndex(r, g, b), viewW * viewH);
}

void Host::drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b)
{
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    int x1 = x0 + (int)w, y1 = y0 + (int)h;
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 > viewW)
    {
        x1 = viewW;
    }
    if (y1 > viewH)
    {
        y1 = viewH;
    }
    if (x0 >= x1 || y0 >= y1)
    {
        return;
    }
    uint8_t c = colorIndex(r, g, b);
    for (int yy = y0; yy < y1; yy++)
    {
        memset(frame + yy * viewW + x0, c, x1 - x0);
    }
}

void Host::drawCharacter(const unsigned char grid[6][6][3], float x, float y)
{
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    for (int gy = 0; gy < CHARACTER_HEIGHT; gy++)
    {
        int py = y0 + gy;
        if (py < 0 || py >= viewH)
        {
            continue;
        }
        for (int gx = 0; gx < CHARACTER_WIDTH; gx++)
        {
            int px = x0 + gx;
            const unsigned char *p = grid[gy][gx];
            // Black is the pattern's empty pixel, as in the library's own ports
            if (px < 0 || px >= viewW || (p[0] == 0 && p[1] == 0 && p[2] == 0))
            {
                continue;
            }
            frame[py * viewW + px] = colorIndex(p[0], p[1], p[2]);
        }
    }
}

float Host::audioTime() const
{
    return (micros() - audioStartUs) / 1000000.0f;
}

void Host::playTone(float freq, float duration, float when)
{
    TONE_LOCK();
    Tone &t = tones[toneNext];
    t.freq = freq;
    t.duration = duration;
    t.when = when;
    toneNext = (toneNext + 1) % TONE_COUNT;
    TONE_UNLOCK();
}

void Host::stopTone()
{
    TONE_LOCK();
    for (Tone &t : tones)
    {
        t.when = FLT_MAX;
    }
    noteUntil = 0;
    TONE_UNLOCK();
}

// As in the library's M5Stack port: of the notes now due, the latest sounds and the rest are
// dropped, since the buzzer plays one at a time
void Host::soundTick()
{
    if (!active)
    {
        return;
    }
    float now = audioTime();
    TONE_LOCK();
    int due = -1;
    float latest = -1;
    for (int i = 0; i < TONE_COUNT; i++)
    {
        Tone &t = tones[i];
        if (t.when <= now)
        {
            if (t.when > latest)
            {
                due = i;
                latest = t.when;
            }
            t.when = FLT_MAX;
        }
    }
    uint16_t hz = noteHz;
    if (due >= 0)
    {
        hz = (uint16_t)tones[due].freq;
        noteUntil = tones[due].when + tones[due].duration;
    }
    else if (now >= noteUntil)
    {
        hz = 0;
    }
    TONE_UNLOCK();
    if (hz != noteHz)
    {
        noteHz = hz;
        sound.playNote(hz);
    }
}

void Host::readPins()
{
    pins.up.loop();
    pins.down.loop();
    pins.left.loop();
    pins.right.loop();
    pins.ok.loop();
}

Button &Host::menuButton()
{
    switch (cart->menuKey)
    {
    case CART_KEY_DOWN:
        return pins.down;
    case CART_KEY_LEFT:
        return pins.left;
    case CART_KEY_RIGHT:
        return pins.right;
    default:
        return pins.up;
    }
}

void Host::loop()
{
    readPins();
#ifdef CRISP_PC
    soundTick();
#endif
    if (menuButton().wasPressed())
    {
        pause();
        if (!active)
        {
            return;
        }
    }
    uint64_t now = micros();
    if ((int64_t)(now - nextFrameUs) < 0)
    {
        vTaskDelay(1);
        return;
    }
    nextFrameUs += FRAME_US;
    if ((int64_t)(now - nextFrameUs) > (int64_t)FRAME_US * MAX_LATE)
    {
        nextFrameUs = now + FRAME_US;
    }

    bool up = pins.up.isDown(), down = pins.down.isDown();
    bool left = pins.left.isDown(), right = pins.right.isDown();
    bool ok = pins.ok.isDown();
    if (waitRelease)
    {
        if (up || down || left || right || ok)
        {
            up = down = left = right = ok = false;
        }
        else
        {
            waitRelease = false;
        }
    }
    switch (cart->menuKey)
    {
    case CART_KEY_UP:
        up = false;
        break;
    case CART_KEY_DOWN:
        down = false;
        break;
    case CART_KEY_LEFT:
        left = false;
        break;
    case CART_KEY_RIGHT:
        right = false;
        break;
    }
    if (cart->controls == CONTROLS_SLIDE)
    {
        int dx = (right ? 1 : 0) - (left ? 1 : 0);
        pointerHeld = dx == 0 ? 0 : (pointerHeld < POINTER_RAMP ? pointerHeld + 1 : POINTER_RAMP);
        pointerX += dx * (POINTER_SLOW + (POINTER_FAST - POINTER_SLOW) * pointerHeld / POINTER_RAMP);
        pointerX = constrain(pointerX, 0.0f, (float)viewW - 1);
        left = right = up = down = false;
    }
    else if (cart->controls == CONTROLS_TAP)
    {
        left = right = up = down = false;
    }
    crispFrame(left, right, up, down, ok, pointerX, pointerY);
    if (cart->controls == CONTROLS_SLIDE)
    {
        drawPointer();
    }
    statFrames++;

    int hi = crispHiScore(index);
    if (hi != shownHi)
    {
        shownHi = hi;
        scored(index);
        drawHi();
    }

    // Late, the frame is skipped so the game keeps its speed; but never more than a few in a row
    if ((int64_t)(micros() - nextFrameUs) < 0 || ++lateFrames >= MAX_LATE)
    {
        lateFrames = 0;
        present();
    }

#ifdef DEV_MODE
    unsigned long ms = millis();
    if (ms - statSince >= 5000)
    {
        Serial.printf("%.1f fps, %.1f shown\n", statFrames * 1000.0f / (ms - statSince),
                      statShown * 1000.0f / (ms - statSince));
        statFrames = statShown = 0;
        statSince = ms;
    }
#endif
}

// A finger needs no mark, but a pointer moved by the arrows does: a caret on the bottom edge,
// drawn into the frame after the game, so the game never collides with it
void Host::drawPointer()
{
    int x = (int)pointerX;
    uint8_t c = colorIndex(0xe9, 0x1e, 0x63);
    for (int dy = 0; dy < 3; dy++)
    {
        int y = viewH - 1 - dy;
        for (int dx = -(2 - dy); dx <= 2 - dy; dx++)
        {
            if (x + dx >= 0 && x + dx < viewW)
            {
                frame[y * viewW + x + dx] = c;
            }
        }
    }
}

void Host::present()
{
    statShown++;
    bool writing = false;
    int bands = (outH + BAND_ROWS - 1) / BAND_ROWS;
    for (int bi = 0; bi < bands; bi++)
    {
        int oy0 = bi * BAND_ROWS;
        int rows = outH - oy0 < BAND_ROWS ? outH - oy0 : BAND_ROWS;
        int sy0 = yMap[oy0], sy1 = yMap[oy0 + rows - 1];
        // Only the bands whose rows changed go out: most frames move a few sprites
        uint32_t h = fnv(2166136261u, frame + sy0 * viewW, (sy1 - sy0 + 1) * viewW);
        h = fnv(h, (const uint8_t *)palette, paletteCount * 2);
        if (!repaintAll && h == bandHash[bi])
        {
            continue;
        }
        bandHash[bi] = h;
        if (!writing)
        {
            tft.startWrite();
            writing = true;
        }
        uint16_t *buf = band[bandBuffer];
        for (int r = 0; r < rows; r++)
        {
            uint16_t *out = buf + r * outW;
            if (r > 0 && yMap[oy0 + r] == yMap[oy0 + r - 1])
            {
                memcpy(out, out - outW, outW * sizeof(uint16_t));
                continue;
            }
            const uint8_t *src = frame + yMap[oy0 + r] * viewW;
            for (int x = 0; x < outW; x++)
            {
                out[x] = palette[src[xMap[x]]];
            }
        }
        if (dma)
        {
            tft.pushImageDMA(gameX, gameY + oy0, outW, rows, buf);
            bandBuffer ^= 1;
        }
        else
        {
            tft.pushImage(gameX, gameY + oy0, outW, rows, buf);
        }
    }
    repaintAll = false;
    if (writing)
    {
        if (dma)
        {
            tft.dmaWait();
        }
        tft.endWrite();
    }
}

void Host::drawStrip()
{
    int16_t cx = DISPLAY_W / 2;
    tft.fillRect(0, stripY, DISPLAY_W, DISPLAY_H - stripY, Theme::BG);
    if (gameY > 0)
    {
        tft.drawRect(gameX - 2, gameY - 2, outW + 4, outH + 4, Theme::FRAME_DIM);
    }
    const char *menuKey = KEY_NAMES[cart->menuKey];
    char line[48];
    tft.setTextDatum(TC_DATUM);
    if (DISPLAY_H - stripY < 60)
    {
        tft.setTextSize(1);
        tft.setTextColor(Theme::TEXT_DIM);
        snprintf(line, sizeof(line), "%s  -  %s: menu", crispTitle(index), menuKey);
        tft.drawString(line, cx, stripY + 6);
        return;
    }
    int16_t y = stripY + 10;
    tft.setTextColor(Theme::TEXT);
    tft.setTextSize(2);
    tft.drawString(crispTitle(index), cx, y);
    y += 22;
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_DIM);
    tft.drawString(cart->author, cx, y);
    y += 18;

    tft.setTextColor(Theme::TEXT_2);
    const char *keys = cart->controls == CONTROLS_ARROWS  ? "OK: tap   Arrows: move"
                       : cart->controls == CONTROLS_SLIDE ? "Left/Right: slide   OK: tap"
                                                          : "OK: tap or hold";
    tft.drawString(keys, cx, y);
    snprintf(line, sizeof(line), "%s: menu", menuKey);
    tft.drawString(line, cx, y + 12);
    drawHi();
}

void Host::drawHi()
{
    if (DISPLAY_H - stripY < 90)
    {
        return;
    }
    int16_t y = stripY + 78;
    tft.fillRect(0, y, DISPLAY_W, 10, Theme::BG);
    if (shownHi <= 0)
    {
        return;
    }
    char line[24];
    snprintf(line, sizeof(line), "Best %d", shownHi);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(Theme::GOLD);
    tft.drawString(line, DISPLAY_W / 2, y);
}

void Host::drawMenu(uint8_t row)
{
    uint8_t count = launcherShown ? ROW_COUNT : ROW_COUNT - 1;
    int16_t w = 150, h = 16 + count * 16;
    int16_t x = (DISPLAY_W - w) / 2;
    int16_t y = gameY + (outH - h) / 2;
    if (y < 4)
    {
        y = 4;
    }
    tft.fillRect(x, y, w, h, Theme::CARD);
    tft.drawRect(x, y, w, h, Theme::SELECT);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    for (uint8_t i = 0; i < count; i++)
    {
        int16_t ry = y + 10 + i * 16;
        bool on = i == row;
        tft.setTextColor(on ? Theme::SELECT : Theme::TEXT);
        tft.drawString(on ? ">" : " ", x + 14, ry);
        tft.drawString(MENU_NAMES[i], x + 26, ry);
    }
}

// The game stands still until Resume; its frame is drawn again over the menu
void Host::pause()
{
    stopTone();
    noteHz = 0;
    sound.playNote(0);
    uint8_t count = launcherShown ? ROW_COUNT : ROW_COUNT - 1;
    uint8_t row = ROW_RESUME;
    drawMenu(row);
    pins.up.wasPressed();
    pins.down.wasPressed();
    pins.ok.wasPressed();
    pins.left.wasPressed();
    pins.right.wasPressed();
    while (true)
    {
        readPins();
        bool upPressed = pins.up.wasPressed(), downPressed = pins.down.wasPressed();
        bool back = pins.left.wasPressed();
        pins.right.wasPressed();
        if (upPressed || downPressed)
        {
            row = (uint8_t)((row + count + (upPressed ? -1 : 1)) % count);
            sound.playMenu();
            drawMenu(row);
        }
        bool chosen = pins.ok.wasPressed();
        if (back)
        {
            chosen = true;
            row = ROW_RESUME;
        }
        if (chosen)
        {
            sound.playMenu();
            switch (row)
            {
            case ROW_RESUME:
                break;
            case ROW_RESTART:
                tft.fillRect(0, 0, DISPLAY_W, stripY, Theme::BG);
                launch(index);
                break;
            case ROW_QUIT:
                active = false;
                quit(index);
                return;
            case ROW_LAUNCHER:
                returnToLauncher();
                break;
            }
            waitRelease = true;
            tft.fillRect(0, 0, DISPLAY_W, stripY, Theme::BG);
            drawStrip();
            repaintAll = true;
            present();
            nextFrameUs = micros();
            return;
        }
        vTaskDelay(10);
    }
}
