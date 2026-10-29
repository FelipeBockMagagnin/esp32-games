#include "SettingsScreen.h"
#include "../gfx/Themes.h"
#include "../gfx/Widgets.h"

enum Row : uint8_t
{
    ROW_SOUND,
    ROW_DIM,
    ROW_THEME,
    ROW_EGG,
    ROW_RESET,
    ROW_COUNT
};

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"On", "Quiet", "Off"};
static const char *const SOUND_NOTES[(uint8_t)SoundMode::COUNT] = {"Every sound", "Only its calls and big days",
                                                                   "Silent"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Never"};

void SettingsScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    Screen::onEnter(tft);
}

void SettingsScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : ROW_COUNT - 1;
    game.sound.playMenu();
    forceFrame();
}

void SettingsScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    game.sound.playMenu();
    forceFrame();
}

bool SettingsScreen::onBackPress()
{
    game.screens.closeModal();
    return true;
}

void SettingsScreen::onConfirmPress()
{
    switch (selected)
    {
    case ROW_SOUND:
        settings.sound = (settings.sound + 1) % (uint8_t)SoundMode::COUNT;
        break;
    case ROW_DIM:
        settings.dimAfter = (settings.dimAfter + 1) % DIM_OPTION_COUNT;
        break;
    case ROW_THEME:
        settings.theme = (settings.theme + 1) % THEME_COUNT;
        changed();
        game.sound.playMenu();
        game.screens.repaint();
        return;
    case ROW_EGG:
        if (!game.pet.canLeaveEgg())
        {
            game.sound.playError();
            return;
        }
        game.sound.playMenu();
        game.screens.replaceModal(&farewellScreen);
        return;
    case ROW_RESET:
        game.sound.playMenu();
        game.screens.replaceModal(&resetScreen);
        return;
    }
    changed();
    game.sound.playMenu();
    forceFrame();
}

void SettingsScreen::drawScene(Canvas &c) const
{
    const Pet &pet = game.pet;
    uint8_t mode = settings.sound < (uint8_t)SoundMode::COUNT ? settings.sound : 0;
    const ThemeDef &theme = THEMES[Theme::getActive()];
    char eggNote[40];
    if (pet.canLeaveEgg())
    {
        snprintf(eggNote, sizeof(eggNote), "%s keeps a page in the Album", pet.getName());
    }
    else
    {
        snprintf(eggNote, sizeof(eggNote), "A grown pet, a week on, close");
    }
    struct Line
    {
        const char *name;
        const char *value;
        const char *note;
    };
    const Line lines[ROW_COUNT] = {
        {"Sound", SOUND_NAMES[mode], SOUND_NOTES[mode]},
        {"Screen dim", DIM_NAMES[settings.dimAfter < DIM_OPTION_COUNT ? settings.dimAfter : DIM_DEFAULT],
         "After no button is pressed"},
        {"Theme", theme.name, theme.note},
        {"Pass on an egg", pet.canLeaveEgg() ? "Ready" : "Not yet", eggNote},
        {"Reset game", "", "Erase the pet and the Album"},
    };
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        float y = 34 + i * 52;
        if (!c.reaches(y - 3, y + 50))
        {
            continue;
        }
        bool on = i == selected;
        if (on)
        {
            c.fillRoundRect(8, y - 2, 224, 48, 16, Theme::rgb(i == ROW_RESET ? ALERT : ACCENT));
        }
        Widgets::card(c, 10, y, 220, 44, 14);
        bool dim = i == ROW_EGG && !pet.canLeaveEgg();
        c.text(22, y + 7, lines[i].name, FONT_BODY, Theme::rgb(i == ROW_RESET ? ALERT : dim ? INK_FAINT : INK));
        c.text(22, y + 25, lines[i].note, FONT_SMALL, Theme::rgb(INK_SOFT));
        if (lines[i].value[0] != '\0')
        {
            float vw = Canvas::textWidth(lines[i].value, FONT_SMALL) + 16;
            c.fillRoundRect(220 - vw, y + 6, vw, 17, 8.5f, Theme::rgb(on ? ACCENT : WALL_TOP));
            c.text(220 - vw / 2, y + 8, lines[i].value, FONT_SMALL, Theme::rgb(on ? ACCENT_INK : INK_SOFT),
                   Canvas::CENTER);
        }
        if (i == ROW_THEME)
        {
            static const uint8_t SWATCH[] = {BODY_A, BODY_B, BODY_C, BODY_D, BLUSH, GOLD};
            for (uint8_t k = 0; k < sizeof(SWATCH); k++)
            {
                c.fillCircle(170 + k * 10, y + 32, 3.8f, Theme::rgb(SWATCH[k]));
            }
        }
    }
    Widgets::hint(c, 304, "up/down choose  -  red changes  -  left back");
}
