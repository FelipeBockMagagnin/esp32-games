#include "AlbumScreen.h"
#include <math.h>
#include "../art/Rig.h"
#include "../gfx/Widgets.h"

void AlbumScreen::onEnter(TFT_eSPI &tft)
{
    selected = game.pet.getForm();
    selectedAt = millis();
    Screen::onEnter(tft);
}

void AlbumScreen::move(int8_t dir)
{
    selected = (selected + FORM_COUNT + dir) % FORM_COUNT;
    selectedAt = millis();
    game.sound.playMenu();
    forceFrame();
}

void AlbumScreen::onUpPress()
{
    move(-1);
}

void AlbumScreen::onDownPress()
{
    move(1);
}

bool AlbumScreen::onRepeat(Key key)
{
    if (key != Key::UP && key != Key::DOWN)
    {
        return false;
    }
    move(key == Key::UP ? -1 : 1);
    return true;
}

static Pose framePose(float x, float ground, float scale, unsigned long now, bool seen)
{
    Pose p;
    p.x = x;
    p.ground = ground;
    p.scale = scale;
    p.eye = Eye::HAPPY;
    p.mouth = Mouth::W;
    p.blush = 0.8f;
    p.silhouette = !seen;
    p.shadow = false;
    float b = sinf(now / 380.0f) * 0.03f;
    p.squashY = 1 + b;
    p.squashX = 1 - b;
    p.sway = sinf(now / 420.0f);
    return p;
}

void AlbumScreen::drawScene(Canvas &c) const
{
    const Album &album = game.album;
    unsigned long now = frameTime();
    uint16_t ink = Theme::rgb(INK), soft = Theme::rgb(INK_SOFT);

    if (c.reaches(32, 150))
    {
        Widgets::card(c, 10, 32, 220, 116, 16);
        for (uint8_t i = 0; i < FORM_COUNT; i++)
        {
            float cx = 38 + (i % 4) * 55, cy = 62 + (i / 4) * 56;
            bool on = i == selected, seen = album.hasSeen((FormId)i);
            float since = (now - selectedAt) / 1000.0f;
            float pop = on ? 1 + 0.12f * expf(-since * 6) * sinf(since * 30) : 1;
            if (on)
            {
                c.fillCircle(cx, cy, 25 * pop, Theme::rgb(ACCENT));
            }
            c.fillCircle(cx, cy, 22 * pop, Theme::rgb(seen ? WALL_TOP : CARD_EDGE));
            const FormRig &rig = FORM_RIG[i];
            float scale = 30.0f / (rig.w > rig.h ? rig.w : rig.h) * pop;
            drawPet(c, (FormId)i, framePose(cx, cy + rig.h * scale * 0.5f, scale, now, seen));
        }
    }

    FormId form = (FormId)selected;
    bool seen = album.hasSeen(form);
    if (c.reaches(156, 240))
    {
        Widgets::card(c, 10, 156, 220, 82, 16);
        c.text(120, 164, seen ? FORMS[form].name : "???", FONT_TITLE, seen ? ink : Theme::rgb(INK_FAINT), Canvas::CENTER);
        c.text(120, 190, STAGE_NAMES[FORMS[form].stage], FONT_SMALL, soft, Canvas::CENTER);
        c.text(120, 206, seen ? FORMS[form].note : "Not met yet", FONT_BODY, seen ? ink : Theme::rgb(INK_FAINT),
               Canvas::CENTER);
        char buf[40];
        snprintf(buf, sizeof(buf), "Met %u of %u  -  Generation %u", (unsigned)album.seenCount(), (unsigned)FORM_COUNT,
                 (unsigned)album.generation + 1);
        c.text(120, 223, buf, FONT_SMALL, soft, Canvas::CENTER);
    }

    if (c.reaches(246, 304))
    {
        Widgets::card(c, 10, 246, 220, 56, 16);
        c.text(22, 252, "Memories", FONT_BODY, ink);
        uint8_t shown = 0;
        for (uint8_t i = 0; i < Album::MEMORY_COUNT && shown < 2; i++)
        {
            const Album::Memory &m = album.past[i];
            if (m.form >= FORM_COUNT)
            {
                continue;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "%s the %s", m.name, FORMS[m.form].name);
            float y = 270 + shown * 14;
            drawIcon(c, Icon::HEART, 28, y + 6, 10, Theme::rgb(BLUSH), Theme::rgb(SHINE));
            c.text(38, y, buf, FONT_SMALL, soft);
            shown++;
        }
        if (shown == 0)
        {
            c.text(22, 272, "A grown pet can pass on an egg", FONT_SMALL, soft);
            c.text(22, 285, "and keep a page here.", FONT_SMALL, soft);
        }
    }
    Widgets::hint(c, 308, "up/down to browse");
}
