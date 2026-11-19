#include "Player.h"
#include "Launcher.h"
#include "../emu/Saves.h"
#include "../ui/Theme.h"

// The Game Boy's 59.73 Hz
static const unsigned long FRAME_US = 16743;
static const unsigned long MENU_HOLD_MS = 1000;
// Battery RAM is saved this long after the game last wrote it, so a save never lands mid-write
static const uint32_t SAVE_QUIET_FRAMES = 300;
static const unsigned long STATUS_MS = 2000;
static const uint8_t SELECT_FRAMES = 8;
static const uint8_t START_FRAMES = 8;

static const char *const MENU_NAMES[] = {"Resume", "Press Select", "Swap A and B", "Save now", "Quit to list",
                                         "Game menu"};

Player::Player(TFT_eSPI &tft, const Pack &pack, const char *nameSpace, SoundManager &sound, void (*quit)(),
               void (*toLauncher)())
    : tft(tft), pack(pack), nameSpace(nameSpace), sound(sound), quit(quit), toLauncher(toLauncher)
{
}

bool Player::begin()
{
    dma = tft.initDMA();
    return dma;
}

// The panel takes RGB565 high byte first, and the band goes out as it sits in memory
static uint16_t panelOrder(uint16_t c)
{
    return (uint16_t)(c << 8 | c >> 8);
}

bool Player::start(uint16_t index)
{
    if (index >= pack.count())
    {
        return false;
    }
    game = pack.entry(index);
    if (!gb.start(pack.rom(game), onLine, this))
    {
        return false;
    }
    if (gb.saveData() != nullptr)
    {
        Saves::load(nameSpace, game.id, gb.saveData(), gb.saveSize());
    }
    gb.clean();
    gb.setFrameSkip(true);
    shades[0] = panelOrder(Theme::SHADE_0);
    shades[1] = panelOrder(Theme::SHADE_1);
    shades[2] = panelOrder(Theme::SHADE_2);
    shades[3] = panelOrder(Theme::SHADE_3);

    okIsB = game.okIsB;
    paused = false;
    // The Confirm press that started the game must not reach it
    waitRelease = true;
    selectFrames = 0;
    startFrames = 0;
    startChord = otherChord = false;
    status = nullptr;
    launcherShown = launcherInstalled();
    tft.fillRect(0, 0, Scale::OUT_W, Scale::OUT_H, Theme::SHADE_0);
    drawStrip();
    nextFrameUs = micros();
    statSince = millis();
    statFrames = 0;
    return true;
}

void Player::onLine(void *context, const uint8_t *pixels, uint8_t line)
{
    ((Player *)context)->line(pixels, line);
}

void Player::line(const uint8_t *pixels, uint8_t line)
{
    int row = Scale::firstRow(line);
    int bandStart = row / BAND_ROWS * BAND_ROWS;
    uint16_t *buf = band[bandBuffer];
    uint16_t *out = buf + (row - bandStart) * Scale::OUT_W;
    Scale::line(pixels, shades, out);
    if (Scale::rowCount(line) == 2)
    {
        memcpy(out + Scale::OUT_W, out, Scale::OUT_W * sizeof(uint16_t));
    }
    // 16 lines fill a band: push it, and draw the next lines into the other buffer meanwhile
    if (line % 16 != 15)
    {
        return;
    }
    if (!writing)
    {
        tft.startWrite();
        writing = true;
    }
    if (dma)
    {
        tft.pushImageDMA(0, bandStart, Scale::OUT_W, BAND_ROWS, buf);
        bandBuffer ^= 1;
    }
    else
    {
        tft.pushImage(0, bandStart, Scale::OUT_W, BAND_ROWS, buf);
    }
    if (line == Scale::SRC_H - 1)
    {
        finishFrame();
    }
}

void Player::finishFrame()
{
    if (!writing)
    {
        return;
    }
    if (dma)
    {
        tft.dmaWait();
    }
    tft.endWrite();
    writing = false;
}

uint8_t Player::joypad(const Pad &pad)
{
    uint8_t held = 0;
    uint8_t primary = okIsB ? JOYPAD_B : JOYPAD_A;
    uint8_t secondary = okIsB ? JOYPAD_A : JOYPAD_B;
    if (!startChord)
    {
        held |= (pad.up ? JOYPAD_UP : 0) | (pad.down ? JOYPAD_DOWN : 0);
    }
    if (!otherChord)
    {
        held |= (pad.left ? JOYPAD_LEFT : 0) | (pad.right ? JOYPAD_RIGHT : 0);
    }
    else if (pad.left && pad.right)
    {
        held |= secondary;
    }
    if (pad.ok)
    {
        held |= primary;
    }
    if (selectFrames > 0)
    {
        selectFrames--;
        held |= JOYPAD_SELECT;
    }
    if (startFrames > 0)
    {
        startFrames--;
        held |= JOYPAD_START;
    }
    return held;
}

void Player::loop(const Pad &pad)
{
    if (!gb.running())
    {
        return;
    }
    unsigned long now = millis();
    if (paused)
    {
        menuInput(pad);
        return;
    }
    if (waitRelease)
    {
        if (pad.up || pad.down || pad.left || pad.right || pad.ok)
        {
            gb.setButtons(0);
        }
        else
        {
            waitRelease = false;
        }
    }

    if (pad.left && pad.right)
    {
        otherChord = true;
    }
    else if (!pad.left && !pad.right)
    {
        otherChord = false;
    }
    if (pad.up && pad.down)
    {
        if (!startChord)
        {
            startChord = true;
            chordSince = now;
        }
        else if (now - chordSince >= MENU_HOLD_MS)
        {
            openMenu();
            return;
        }
    }
    else if (startChord && !pad.up && !pad.down)
    {
        // Start goes out on release, so holding for the menu never pauses the game behind it
        startChord = false;
        if (!waitRelease)
        {
            startFrames = START_FRAMES;
        }
    }

    unsigned long us = micros();
    if ((long)(us - nextFrameUs) < 0)
    {
        return;
    }
    // Running late, it slows down rather than racing to catch up
    nextFrameUs = (long)(us - nextFrameUs) > (long)FRAME_US ? us + FRAME_US : nextFrameUs + FRAME_US;

    gb.setButtons(waitRelease ? 0 : joypad(pad));
    gb.runFrame();
    finishFrame();
    sound.playNote(gb.sound().frame());

    if (gb.crashed())
    {
        sound.playNote(0);
        showStatus("The game crashed");
        openMenu();
        return;
    }
    if (gb.dirty() && gb.quietFrames() >= SAVE_QUIET_FRAMES)
    {
        save(true);
    }
    if (status != nullptr && (long)(now - statusUntil) >= 0)
    {
        status = nullptr;
        drawStrip();
    }

#ifdef DEV_MODE
    statFrames++;
    if (now - statSince >= 5000)
    {
        Serial.printf("%.1f fps\n", statFrames * 1000.0f / (now - statSince));
        statFrames = 0;
        statSince = now;
    }
#endif
}

void Player::save(bool quietly)
{
    if (gb.saveData() == nullptr)
    {
        if (!quietly)
        {
            showStatus("This game has no save");
        }
        return;
    }
    gb.clean();
    Saves::Result result = Saves::store(nameSpace, game.id, gb.saveData(), gb.saveSize());
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

void Player::showStatus(const char *text)
{
    status = text;
    statusUntil = millis() + STATUS_MS;
    if (paused)
    {
        drawMenu();
    }
    else
    {
        drawStrip();
    }
}

void Player::drawStrip()
{
    tft.fillRect(0, STRIP_Y, Scale::OUT_W, TFT_HEIGHT - STRIP_Y, Theme::BG);
    tft.drawFastHLine(0, STRIP_Y, Scale::OUT_W, Theme::FRAME_DIM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::TEXT);
    tft.setTextSize(2);
    tft.drawString(game.title, 120, STRIP_Y + 8);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_DIM);
    tft.drawString(game.author, 120, STRIP_Y + 28);

    tft.setTextDatum(TL_DATUM);
    drawControl(okIsB ? "B" : "A", "OK", 8, STRIP_Y + 46);
    drawControl(okIsB ? "A" : "B", "Left+Right", 8, STRIP_Y + 60);
    drawControl("Start", "Up+Down", 116, STRIP_Y + 46);
    drawControl("Menu", "hold Up+Down", 116, STRIP_Y + 60);

    if (status != nullptr)
    {
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(Theme::SELECT);
        tft.drawString(status, 120, STRIP_Y + 84);
    }
}

void Player::drawControl(const char *action, const char *keys, int16_t x, int16_t y)
{
    tft.setTextColor(Theme::TEXT);
    tft.drawString(action, x, y);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString(keys, x + 36, y);
}

void Player::drawMenu()
{
    tft.fillRect(0, STRIP_Y, Scale::OUT_W, TFT_HEIGHT - STRIP_Y, Theme::CARD);
    tft.drawFastHLine(0, STRIP_Y, Scale::OUT_W, Theme::SELECT);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    uint8_t shown = 0;
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        if (i == ROW_LAUNCHER && !launcherShown)
        {
            continue;
        }
        int16_t y = STRIP_Y + 6 + shown * 13;
        bool on = i == menuRow;
        tft.setTextColor(on ? Theme::SELECT : Theme::TEXT);
        tft.drawString(on ? ">" : " ", 16, y);
        tft.drawString(MENU_NAMES[i], 28, y);
        if (i == ROW_SWAP)
        {
            tft.setTextColor(Theme::TEXT_DIM);
            tft.drawString(okIsB ? "OK is B" : "OK is A", 150, y);
        }
        shown++;
    }
    if (status != nullptr)
    {
        tft.setTextColor(Theme::SELECT);
        tft.drawString(status, 150, STRIP_Y + 6);
    }
}

void Player::openMenu()
{
    finishFrame();
    sound.playNote(0);
    gb.setButtons(0);
    paused = true;
    waitRelease = true;
    startChord = otherChord = false;
    menuRow = gb.crashed() ? ROW_QUIT : ROW_RESUME;
    // A battery game is saved whenever the menu opens: quitting by power-off is the usual way
    if (gb.dirty())
    {
        save(true);
    }
    drawMenu();
}

void Player::menuInput(const Pad &pad)
{
    // The chord that opened the menu is still held for a moment: wait it out
    if (waitRelease)
    {
        if (!(pad.up || pad.down || pad.left || pad.right || pad.ok))
        {
            waitRelease = false;
        }
        return;
    }
    if (pad.upPressed || pad.downPressed)
    {
        int8_t step = pad.upPressed ? -1 : 1;
        do
        {
            menuRow = (uint8_t)((menuRow + ROW_COUNT + step) % ROW_COUNT);
        } while ((menuRow == ROW_LAUNCHER && !launcherShown) || (gb.crashed() && menuRow < ROW_QUIT));
        sound.playMenu();
        drawMenu();
        return;
    }
    if (!pad.okPressed)
    {
        return;
    }
    sound.playMenu();
    switch (menuRow)
    {
    case ROW_SELECT:
        selectFrames = SELECT_FRAMES;
        [[fallthrough]];
    case ROW_RESUME:
        paused = false;
        waitRelease = true;
        status = nullptr;
        drawStrip();
        nextFrameUs = micros();
        break;
    case ROW_SWAP:
        okIsB = !okIsB;
        drawMenu();
        break;
    case ROW_SAVE:
        save(false);
        break;
    case ROW_QUIT:
        end(false);
        break;
    case ROW_LAUNCHER:
        end(true);
        break;
    }
}

void Player::end(bool launcher)
{
    if (gb.dirty() && !gb.crashed())
    {
        save(true);
    }
    sound.playNote(0);
    gb.stop();
    paused = false;
    if (launcher)
    {
        toLauncher();
    }
    quit();
}
