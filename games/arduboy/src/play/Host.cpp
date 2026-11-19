#include "Host.h"
#include <Arduboy2.h>
#include <EEPROM.h>
#include "ArduboyHost.h"
#include "Launcher.h"
#include "UI.h"
#include "../save/Saves.h"
#include "../ui/Theme.h"

uint8_t abEeprom[AB_EEPROM_SIZE];
volatile uint32_t abEepromWrites = 0;
void (*abHostTick)() = nullptr;

static Host *host = nullptr;

void abHostPaint(const uint8_t *image, uint8_t flags)
{
    host->paint(image, flags);
}

uint8_t abHostButtons()
{
    return host->buttons();
}

void abHostIdle()
{
    host->idle();
}

void abHostTone(uint8_t channel, uint16_t hz)
{
    host->tone(channel, hz);
}

static const unsigned long MENU_HOLD_MS = 1000;
// A game writes its EEPROM a byte at a time: it is stored once the writes have stopped this long
static const unsigned long SAVE_QUIET_MS = 1500;
static const unsigned long STATUS_MS = 2000;

static const char *const MENU_NAMES[] = {"Resume", "Swap A and B", "Restart game", "Quit to list", "Game menu"};

// What a new Arduboy's EEPROM holds: the system bytes with the boot logo off and sound on, the
// games' space erased
static void blankEeprom()
{
    memset(abEeprom, 0xFF, sizeof(abEeprom));
    memset(abEeprom, 0, EEPROM_STORAGE_SPACE_START);
    abEeprom[2] = 0xFF;
}

// The panel takes RGB565 high byte first, and the band goes out as it sits in memory
static uint16_t panelOrder(uint16_t c)
{
    return (uint16_t)(c << 8 | c >> 8);
}

Host::Host(TFT_eSPI &tft, Pins pins, const char *nameSpace, SoundManager &sound, void (*quit)(uint16_t index),
           void (*restart)(uint16_t index))
    : tft(tft), pins(pins), nameSpace(nameSpace), sound(sound), quit(quit), restart(restart)
{
    host = this;
}

bool Host::begin()
{
    dma = tft.initDMA();
    return dma;
}

void Host::start(uint16_t i)
{
    index = i;
    game = &SKETCHES[i];
    gameX = (DISPLAY_W - OUT_W) / 2;
    gameY = 8;
    stripY = gameY + OUT_H + 12;
    onColor = panelOrder(Theme::PIXEL_ON);
    offColor = panelOrder(Theme::PIXEL_OFF);
    memset(shown, 0, sizeof(shown));
    shownFlags = 0xFF;

    blankEeprom();
    Saves::load(nameSpace, game->id, abEeprom, sizeof(abEeprom));
    savedWrites = seenWrites = abEepromWrites;

    okIsB = game->okIsB;
    // The Confirm press that started the game must not reach it
    waitRelease = true;
    launcherShown = launcherInstalled();
    status = nullptr;

    tft.fillScreen(Theme::BG);
    tft.drawRect(gameX - 2, gameY - 2, OUT_W + 4, OUT_H + 4, Theme::FRAME_DIM);
    tft.fillRect(gameX, gameY, OUT_W, OUT_H, Theme::PIXEL_OFF);
    drawStrip();
    statSince = millis();
    statFrames = 0;
}

void Host::readPins()
{
    pins.up.loop();
    pins.down.loop();
    pins.left.loop();
    pins.right.loop();
    pins.ok.loop();
}

void Host::poll()
{
    unsigned long now = millis();
    if (abHostTick != nullptr)
    {
        abHostTick();
    }
    if (now == lastPollMs)
    {
        return;
    }
    lastPollMs = now;
    readPins();

    bool up = pins.up.isDown(), down = pins.down.isDown();
    if (up && down)
    {
        if (chordSince == 0)
        {
            chordSince = now;
        }
        else if (now - chordSince >= MENU_HOLD_MS)
        {
            chordSince = 0;
            pause();
            return;
        }
    }
    else
    {
        chordSince = 0;
    }

    uint32_t writes = abEepromWrites;
    if (writes != seenWrites)
    {
        seenWrites = writes;
        writtenAt = now;
    }
    else if (writes != savedWrites && now - writtenAt >= SAVE_QUIET_MS)
    {
        saveEeprom(true);
    }

    if (status != nullptr && (long)(now - statusUntil) >= 0)
    {
        status = nullptr;
        drawStrip();
    }
}

uint8_t Host::buttons()
{
    poll();
    bool up = pins.up.isDown(), down = pins.down.isDown();
    bool left = pins.left.isDown(), right = pins.right.isDown();
    bool ok = pins.ok.isDown();
    if (waitRelease)
    {
        if (up || down || left || right || ok)
        {
            return 0;
        }
        waitRelease = false;
        menuChord = otherChord = false;
    }

    // A chord stays one until both its buttons are up, so letting go of one first never sends its
    // partner's direction. Up+Down together is the menu's chord, never a direction.
    if (up && down)
    {
        menuChord = true;
    }
    else if (!up && !down)
    {
        menuChord = false;
    }
    if (left && right)
    {
        otherChord = true;
    }
    else if (!left && !right)
    {
        otherChord = false;
    }

    uint8_t primary = okIsB ? B_BUTTON : A_BUTTON;
    uint8_t secondary = okIsB ? A_BUTTON : B_BUTTON;
    uint8_t held = 0;
    if (!menuChord)
    {
        held |= (up ? UP_BUTTON : 0) | (down ? DOWN_BUTTON : 0);
    }
    if (!otherChord)
    {
        held |= (left ? LEFT_BUTTON : 0) | (right ? RIGHT_BUTTON : 0);
    }
    else if (left && right)
    {
        held |= secondary;
    }
    if (ok)
    {
        held |= primary;
    }
    return held;
}

void Host::idle()
{
    poll();
    vTaskDelay(1);
}

void Host::tone(uint8_t channel, uint16_t hz)
{
    channelHz[channel & 1] = hz;
    sound.playNote(channelHz[0] != 0 ? channelHz[0] : channelHz[1]);
}

void Host::pushPage(uint8_t page, const uint8_t *image, uint8_t flags)
{
    uint16_t *buf = band[bandBuffer];
    bool flipV = flags & AB_FLIP_V, flipH = flags & AB_FLIP_H;
    uint16_t on = flags & AB_INVERT ? offColor : onColor;
    uint16_t off = flags & AB_INVERT ? onColor : offColor;
    uint8_t sourcePage = flipV ? PAGES - 1 - page : page;
    const uint8_t *src = image + sourcePage * 128;
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        uint8_t mask = 1 << (flipV ? 7 - bit : bit);
        uint16_t *out = buf + bit * SCALE * OUT_W;
        for (uint8_t x = 0; x < 128; x++)
        {
            uint8_t column = src[flipH ? 127 - x : x];
            bool lit = flags & AB_OFF ? false : (flags & AB_ALL_ON ? true : (column & mask) != 0);
            uint16_t c = lit ? on : off;
            out[x * 2] = c;
            out[x * 2 + 1] = c;
        }
        memcpy(out + OUT_W, out, OUT_W * sizeof(uint16_t));
    }
    if (dma)
    {
        tft.pushImageDMA(gameX, gameY + page * BAND_ROWS, OUT_W, BAND_ROWS, buf);
        bandBuffer ^= 1;
    }
    else
    {
        tft.pushImage(gameX, gameY + page * BAND_ROWS, OUT_W, BAND_ROWS, buf);
    }
}

void Host::paint(const uint8_t *image, uint8_t flags)
{
    poll();
    bool all = flags != shownFlags;
    bool writing = false;
    for (uint8_t page = 0; page < PAGES; page++)
    {
        // Only the pages that changed go out: most frames move a sprite or two
        uint8_t sourcePage = flags & AB_FLIP_V ? PAGES - 1 - page : page;
        const uint8_t *src = image + sourcePage * 128;
        uint8_t *seen = shown + sourcePage * 128;
        if (!all && memcmp(src, seen, 128) == 0)
        {
            continue;
        }
        memcpy(seen, src, 128);
        if (!writing)
        {
            tft.startWrite();
            writing = true;
        }
        pushPage(page, image, flags);
    }
    shownFlags = flags;
    if (writing)
    {
        if (dma)
        {
            tft.dmaWait();
        }
        tft.endWrite();
    }

#ifdef DEV_MODE
    statFrames++;
    unsigned long now = millis();
    if (now - statSince >= 5000)
    {
        Serial.printf("%.1f fps\n", statFrames * 1000.0f / (now - statSince));
        statFrames = 0;
        statSince = now;
    }
#endif
}

void Host::saveEeprom(bool quietly)
{
    savedWrites = seenWrites = abEepromWrites;
    Saves::Result result = Saves::store(nameSpace, game->id, abEeprom, sizeof(abEeprom));
    if (result == Saves::Result::SAVED)
    {
        if (!quietly)
        {
            showStatus("Saved");
        }
        return;
    }
    showStatus(result == Saves::Result::TOO_BIG ? "Save too big to keep" : "No room left to save");
}

void Host::showStatus(const char *text)
{
    status = text;
    statusUntil = millis() + STATUS_MS;
    drawStrip();
}

void Host::drawStrip()
{
    int16_t cx = DISPLAY_W / 2;
    tft.fillRect(0, stripY, DISPLAY_W, DISPLAY_H - stripY, Theme::BG);
    tft.drawFastHLine(0, stripY, DISPLAY_W, Theme::FRAME_DIM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::TEXT);
    tft.setTextSize(2);
    tft.drawString(game->title, cx, stripY + 8);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_DIM);
    tft.drawString(game->author, cx, stripY + 28);

    tft.setTextColor(Theme::TEXT_2);
    tft.setTextDatum(TL_DATUM);
    drawControl(okIsB ? "B" : "A", "OK", 40, stripY + 46);
    drawControl(okIsB ? "A" : "B", "Left+Right", 40, stripY + 60);
    drawControl("Menu", "hold Up+Down", 168, stripY + 46);

    if (status != nullptr)
    {
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(Theme::SELECT);
        tft.drawString(status, cx, stripY + 78);
    }
}

void Host::drawControl(const char *action, const char *keys, int16_t x, int16_t y)
{
    tft.setTextColor(Theme::TEXT);
    tft.drawString(action, x, y);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString(keys, x + 36, y);
}

void Host::drawMenu(uint8_t row)
{
    tft.fillRect(0, stripY, DISPLAY_W, DISPLAY_H - stripY, Theme::CARD);
    tft.drawFastHLine(0, stripY, DISPLAY_W, Theme::SELECT);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    uint8_t drawn = 0;
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        if (i == ROW_LAUNCHER && !launcherShown)
        {
            continue;
        }
        int16_t y = stripY + 8 + drawn * 14;
        bool on = i == row;
        tft.setTextColor(on ? Theme::SELECT : Theme::TEXT);
        tft.drawString(on ? ">" : " ", 60, y);
        tft.drawString(MENU_NAMES[i], 72, y);
        if (i == ROW_SWAP)
        {
            tft.setTextColor(Theme::TEXT_DIM);
            tft.drawString(okIsB ? "OK is B" : "OK is A", 190, y);
        }
        drawn++;
    }
}

// The game is frozen inside whichever library call brought it here, until Resume
void Host::pause()
{
    sound.playNote(0);
    if (abEepromWrites != savedWrites)
    {
        saveEeprom(true);
    }
    uint8_t row = ROW_RESUME;
    drawMenu(row);
    // The chord that opened the menu is still held: wait it out
    do
    {
        readPins();
        vTaskDelay(5);
    } while (pins.up.isDown() || pins.down.isDown());
    pins.up.wasPressed();
    pins.down.wasPressed();
    pins.ok.wasPressed();

    while (true)
    {
        readPins();
        bool upPressed = pins.up.wasPressed(), downPressed = pins.down.wasPressed();
        if (upPressed || downPressed)
        {
            int8_t step = upPressed ? -1 : 1;
            do
            {
                row = (uint8_t)((row + ROW_COUNT + step) % ROW_COUNT);
            } while (row == ROW_LAUNCHER && !launcherShown);
            sound.playMenu();
            drawMenu(row);
        }
        if (pins.ok.wasPressed())
        {
            sound.playMenu();
            switch (row)
            {
            case ROW_RESUME:
                waitRelease = true;
                drawStrip();
                sound.playNote(channelHz[0] != 0 ? channelHz[0] : channelHz[1]);
                return;
            case ROW_SWAP:
                okIsB = !okIsB;
                drawMenu(row);
                break;
            case ROW_RESTART:
                restart(index);
                break;
            case ROW_QUIT:
                quit(index);
                break;
            case ROW_LAUNCHER:
                returnToLauncher();
                break;
            }
        }
        vTaskDelay(10);
    }
}
