#include "StatsScreen.h"
#include "../art/IdlePose.h"
#include "../gfx/Widgets.h"

static const char *const MOOD_NAMES[] = {"Asleep", "Unwell", "Sleepy", "Sad", "Okay", "Content", "Happy"};
static const uint8_t MOOD_COLORS[] = {WATER, ALERT, WATER, ALERT, INK_FAINT, LEAF, GOLD};

void StatsScreen::drawScene(Canvas &c) const
{
    const Pet &pet = game.pet;
    unsigned long now = frameTime();
    char buf[40], age[16];
    uint16_t ink = Theme::rgb(INK), soft = Theme::rgb(INK_SOFT);

    if (c.reaches(32, 132))
    {
        Widgets::card(c, 10, 32, 220, 98, 16);
        c.fillCircle(56, 81, 40, Theme::rgb(WALL_TOP));
        c.fillEllipse(56, 108, 30, 5, Theme::rgb(RUG), 200);
        drawPet(c, pet.getForm(), idlePose(pet, now, 56, 108, 0.62f));
        c.text(108, 42, pet.getName(), FONT_TITLE, ink);
        snprintf(buf, sizeof(buf), "%s, %s", FORMS[pet.getForm()].name, STAGE_NAMES[pet.getStage()]);
        c.text(108, 68, buf, FONT_BODY, soft);
        formatDuration(pet.getAge(), age, sizeof(age));
        snprintf(buf, sizeof(buf), "Age %s", pet.isEgg() ? "-" : age);
        c.text(108, 88, buf, FONT_SMALL, soft);
        if (pet.getStage() == STAGE_ADULT)
        {
            snprintf(buf, sizeof(buf), "All grown up  -  Gen %u", (unsigned)game.album.generation + 1);
        }
        else
        {
            formatDuration(pet.getSecsToGrow() + 59, age, sizeof(age));
            snprintf(buf, sizeof(buf), "Grows in %s", age);
        }
        c.text(108, 104, buf, FONT_SMALL, soft);
    }

    if (c.reaches(138, 252))
    {
        Widgets::card(c, 10, 138, 220, 112, 16);
        static const char *const LABELS[4] = {"Food", "Joy", "Rest", "Tidy"};
        static const Icon ICONS[4] = {Icon::BOWL, Icon::STAR, Icon::MOON, Icon::SPARKLE};
        static const uint8_t COLORS[4] = {BLUSH, GOLD, WATER, LEAF};
        const uint8_t values[4] = {pet.getNeed(NEED_FOOD), pet.getNeed(NEED_JOY), pet.getNeed(NEED_REST),
                                   pet.getTidy()};
        for (uint8_t i = 0; i < 4; i++)
        {
            float y = 150 + i * 25;
            uint8_t color = values[i] < NEED_LOW ? ALERT : COLORS[i];
            drawIcon(c, ICONS[i], 28, y + 8, 15, Theme::rgb(color), Theme::rgb(CARD));
            c.text(42, y + 1, LABELS[i], FONT_BODY, ink);
            Widgets::meter(c, 82, y + 3, 110, 10, values[i] / (float)NEED_MAX, color);
            snprintf(buf, sizeof(buf), "%u", (unsigned)values[i]);
            c.text(218, y + 2, buf, FONT_SMALL, soft, Canvas::RIGHT);
        }
    }

    if (c.reaches(258, 304))
    {
        Widgets::card(c, 10, 258, 220, 44, 16);
        c.text(22, 266, "Bond", FONT_BODY, ink);
        uint8_t hearts = (uint8_t)((pet.getBond() * 8 + BOND_MAX - 1) / BOND_MAX);
        for (uint8_t i = 0; i < 8; i++)
        {
            float pulse = i < hearts ? 1 + 0.08f * sinf(now / 300.0f + i) : 1;
            drawIcon(c, Icon::HEART, 70 + i * 13, 274, 12 * pulse, Theme::rgb(i < hearts ? BLUSH : CARD_EDGE),
                     Theme::rgb(SHINE));
        }
        const char *mood = MOOD_NAMES[(uint8_t)pet.mood()];
        float mw = Canvas::textWidth(mood, FONT_SMALL) + 16;
        c.fillRoundRect(218 - mw, 266, mw, 17, 8.5f, Theme::rgb(MOOD_COLORS[(uint8_t)pet.mood()]));
        c.text(218 - mw / 2, 268, mood, FONT_SMALL, Theme::rgb(ACCENT_INK), Canvas::CENTER);
        c.text(22, 286, pet.isLightOn() ? "Lamp on" : "Lamp off", FONT_SMALL, soft);
    }
    Widgets::hint(c, 308, "Meals and play shape how it grows");
}
