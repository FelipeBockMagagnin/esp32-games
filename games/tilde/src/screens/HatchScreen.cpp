#include "HatchScreen.h"
#include <string.h>
#include <math.h>
#include "../art/IdlePose.h"
#include "../gfx/Widgets.h"

// A space ends the name, so it comes first after Z
static const char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
static const uint8_t LETTER_COUNT = sizeof(LETTERS) - 1;

void HatchScreen::onEnter(TFT_eSPI &tft)
{
    memset(name, ' ', NAME_LEN);
    name[NAME_LEN] = '\0';
    const char *current = game.pet.getName();
    for (uint8_t i = 0; i < NAME_LEN && current[i] != '\0'; i++)
    {
        char c = current[i];
        name[i] = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
    }
    cursor = 0;
    game.sound.playHatch();
    Screen::onEnter(tft);
}

void HatchScreen::step(int8_t dir)
{
    const char *at = strchr(LETTERS, name[cursor]);
    uint8_t index = at != nullptr ? (uint8_t)(at - LETTERS) : LETTER_COUNT - 1;
    name[cursor] = LETTERS[(index + LETTER_COUNT + dir) % LETTER_COUNT];
    movedAt = millis();
    game.sound.playMenu();
    forceFrame();
}

void HatchScreen::onUpPress()
{
    step(-1);
}

void HatchScreen::onDownPress()
{
    step(1);
}

bool HatchScreen::onBackPress()
{
    cursor = cursor > 0 ? cursor - 1 : 0;
    movedAt = millis();
    forceFrame();
    return true;
}

bool HatchScreen::onRightPress()
{
    cursor = cursor + 1 < NAME_LEN ? cursor + 1 : cursor;
    movedAt = millis();
    forceFrame();
    return true;
}

bool HatchScreen::onRepeat(Key key)
{
    switch (key)
    {
    case Key::UP:
        step(-1);
        return true;
    case Key::DOWN:
        step(1);
        return true;
    case Key::LEFT:
        return onBackPress();
    case Key::RIGHT:
        return onRightPress();
    default:
        return false;
    }
}

void HatchScreen::onConfirmPress()
{
    // Trimmed, and written the way a name is: Momo, not MOMO
    char kept[NAME_LEN + 1] = "";
    uint8_t length = 0;
    for (uint8_t i = 0; i < NAME_LEN; i++)
    {
        if (name[i] != ' ')
        {
            length = i + 1;
        }
    }
    for (uint8_t i = 0; i < length; i++)
    {
        char c = name[i];
        kept[i] = i > 0 && c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
    }
    kept[length] = '\0';
    if (length > 0)
    {
        game.pet.setName(kept);
    }
    game.album.see(game.pet.getForm());
    game.saver.soon(millis());
    game.sound.playVoice(FORM_RIG[game.pet.getForm()].voice);
    game.screens.closeModal();
}

void HatchScreen::drawScene(Canvas &c) const
{
    unsigned long now = frameTime();
    FormId form = game.pet.getForm();
    if (c.reaches(30, 180))
    {
        Widgets::card(c, 30, 34, 180, 140, 20, WALL_TOP);
        c.fillEllipse(120, 152, 50, 8, Theme::rgb(RUG), 200);
        Pose p = idlePose(game.pet, now, 120, 150, 1.2f);
        float hop = fabsf(sinf(now / 420.0f));
        p.lift = hop * 16;
        p.squashY = 1 + (hop - 0.4f) * 0.14f;
        p.squashX = 2 - p.squashY;
        p.eye = (now / 2200) % 3 == 0 ? Eye::STAR : Eye::HAPPY;
        p.mouth = Mouth::W;
        p.blush = 1;
        drawPet(c, form, p);
        for (int i = 0; i < 4; i++)
        {
            float t = fmodf(now / 1600.0f + i / 4.0f, 1.0f);
            float sx = 50 + i * 46 + sinf(i * 2.0f) * 8;
            drawIcon(c, Icon::SPARKLE, sx, 60 + sinf(t * 6.28f + i) * 8, 10 + 4 * sinf(t * 3.14f), Theme::rgb(GOLD),
                     Theme::rgb(GOLD), (uint8_t)(255 * sinf(t * 3.14f)));
        }
    }
    char hello[32];
    snprintf(hello, sizeof(hello), "It's a little %s!", FORMS[form].name);
    c.text(120, 186, hello, FONT_TITLE, Theme::rgb(INK), Canvas::CENTER);
    c.text(120, 212, "What will you call it?", FONT_BODY, Theme::rgb(INK_SOFT), Canvas::CENTER);

    const float tileW = 24, gap = 4, x0 = (Canvas::W - NAME_LEN * tileW - (NAME_LEN - 1) * gap) / 2, y = 236;
    for (uint8_t i = 0; i < NAME_LEN; i++)
    {
        bool on = i == cursor;
        float since = (now - movedAt) / 1000.0f;
        float jump = on ? 6 * expf(-since * 7) * fabsf(sinf(since * 18)) : 0;
        float x = x0 + i * (tileW + gap);
        c.fillRoundRect(x, y + 2 - jump, tileW, 32, 8, Theme::rgb(SHADOW), 90);
        c.fillRoundRect(x, y - jump, tileW, 32, 8, Theme::rgb(on ? ACCENT : CARD));
        char glyph[2] = {name[i] == ' ' ? '_' : name[i], '\0'};
        c.text(x + tileW / 2, y + 6 - jump, glyph, FONT_TITLE,
               Theme::rgb(on ? ACCENT_INK : name[i] == ' ' ? INK_FAINT : INK), Canvas::CENTER);
    }
    Widgets::hint(c, 284, "up/down letter  -  left/right move");
    Widgets::hint(c, 298, "red button: that's its name!");
}
