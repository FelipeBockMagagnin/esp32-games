#include "Screen.h"
#include "../gfx/Icons.h"

char Screen::noticeText[48] = "";
uint8_t Screen::noticeAccent = ACCENT;
unsigned long Screen::noticeAt = 0;
unsigned long Screen::noticeUntil = 0;

static const unsigned long NOTICE_SLIDE_MS = 220;

void Screen::notify(const char *text, uint8_t accent, unsigned long now, unsigned long ms)
{
    snprintf(noticeText, sizeof(noticeText), "%s", text);
    noticeAccent = accent;
    noticeAt = now;
    noticeUntil = now + ms;
}

void Screen::repaint(TFT_eSPI &tft)
{
    display.invalidate();
    frameForced = true;
    render(tft);
}

void Screen::render(TFT_eSPI &tft)
{
    unsigned long now = millis();
    if (!frameForced && now - lastFrame < frameMs)
    {
        return;
    }
    frameForced = false;
    lastFrame = now;
    frameNow = now;
    display.frame(*this, Theme::rgb(BG));
}

uint32_t Screen::keyText(uint32_t key, const char *text)
{
    for (const char *p = text; p != nullptr && *p != '\0'; p++)
    {
        key = keyMix(key, (uint8_t)*p);
    }
    return keyMix(key, 0);
}

uint32_t Screen::headerKey() const
{
    uint32_t key = keyText(keyText(keyText(2166136261u, title()), backLabel), nextLabel);
    unsigned long now = frameNow;
    bool shown = noticeText[0] != '\0' && (long)(now - noticeUntil) < (long)NOTICE_SLIDE_MS;
    if (shown)
    {
        key = keyText(key, noticeText);
        // While it slides every frame differs
        if (now - noticeAt < NOTICE_SLIDE_MS || (long)(noticeUntil - now) < (long)NOTICE_SLIDE_MS)
        {
            key = keyMix(key, now);
        }
    }
    return key;
}

void Screen::draw(Canvas &c) const
{
    drawScene(c);
    if (title() != nullptr)
    {
        drawHeader(c);
    }
}

void Screen::drawHeader(Canvas &c) const
{
    if (!c.reaches(0, HEADER_H))
    {
        return;
    }
    const char *name = title();
    uint16_t faint = Theme::rgb(INK_FAINT);
    int16_t pillW = Canvas::textWidth(name, FONT_BODY) + 28;
    c.fillRoundRect(120 - pillW / 2, 3, pillW, 21, 10.5f, Theme::rgb(CARD));
    c.text(120, 6, name, FONT_BODY, Theme::rgb(INK), Canvas::CENTER);
    if (backLabel[0] != '\0')
    {
        drawIcon(c, Icon::ARROW_LEFT, 8, 13.5f, 9, faint, faint);
        c.text(15, 7, backLabel, FONT_SMALL, faint);
    }
    if (nextLabel[0] != '\0')
    {
        drawIcon(c, Icon::ARROW_RIGHT, 232, 13.5f, 9, faint, faint);
        c.text(225, 7, nextLabel, FONT_SMALL, faint, Canvas::RIGHT);
    }

    // A notice slides down over the bar, and back up as it ends
    unsigned long now = frameNow;
    if (noticeText[0] == '\0' || (long)(now - noticeUntil) >= (long)NOTICE_SLIDE_MS)
    {
        return;
    }
    unsigned long in = now - noticeAt;
    long out = (long)(noticeUntil - now);
    float t = in < NOTICE_SLIDE_MS ? (float)in / NOTICE_SLIDE_MS : 1.0f;
    if (out < (long)NOTICE_SLIDE_MS)
    {
        t = out > 0 ? (float)out / NOTICE_SLIDE_MS : 0.0f;
    }
    float ease = 1 - (1 - t) * (1 - t);
    float y = -24 + ease * 26;
    c.fillRoundRect(6, y, 228, 22, 11, Theme::rgb(noticeAccent));
    c.text(120, y + 4, noticeText, FONT_BODY, Theme::rgb(ACCENT_INK), Canvas::CENTER);
}
