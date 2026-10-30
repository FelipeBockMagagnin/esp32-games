#include "SettingsScreen.h"
#include "Launcher.h"
#include "../ui/Labels.h"
#include "../ui/Themes.h"

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"Ligado", "Baixo", "Desligado"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Nunca"};

SettingsScreen::SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), void (*reset)(),
                               ScreenManager &screenManager, SoundManager &sound, DialogScreen &dialog)
    : Screen("Painel de Controle"),
      settings(settings),
      changed(changed),
      toMenu(toMenu),
      reset(reset),
      screenManager(screenManager),
      sound(sound),
      dialog(dialog),
      about(label(DISPLAY_W / 2, 270, TC_DATUM, Theme::TEXT_DIM))
{
    static const char *const NAMES[ROW_COUNT] = {"Som", "Escurecer tela", "Tema", "Menu de jogos", "Resetar jogo"};
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        SettingRow &row = rows[i];
        int16_t y = ROW_Y + i * ROW_H;
        row.frame = FocusFrame(5, y, 230, ROW_H - 4);
        row.name = label(12, y + 9);
        row.value = label(228, y + 9, TR_DATUM, Theme::TEXT_2);
        row.name.setText(NAMES[i]);
        addElement(&row.name);
        addElement(&row.value);
        addElement(&row.frame);
    }
    rows[ROW_RESET].name.setColor(Theme::BAD);
    rows[ROW_MENU].value.setText(">");
    rows[ROW_RESET].value.setText("...");
    about.setText("NaN Idle - o idle dos devs");
    addElement(&about);
    addTaskbar();
}

void SettingsScreen::refreshRows()
{
    rows[ROW_SOUND].value.setText(SOUND_NAMES[settings.sound < (uint8_t)SoundMode::COUNT ? settings.sound : 0]);
    rows[ROW_DIM].value.setText(DIM_NAMES[settings.dimAfter < DIM_OPTION_COUNT ? settings.dimAfter : DIM_DEFAULT]);
    rows[ROW_THEME].value.setText(activeTheme().name);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].frame.setOn(i == selected);
    }
}

void SettingsScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    menuShown = launcherInstalled();
    SettingRow &menu = rows[ROW_MENU];
    menu.frame.setVisible(menuShown);
    menu.name.setVisible(menuShown);
    menu.value.setVisible(menuShown);
    SettingRow &reset = rows[ROW_RESET];
    int16_t y = ROW_Y + (menuShown ? ROW_RESET : ROW_MENU) * ROW_H;
    reset.frame.setPosition(5, y);
    reset.name.setPosition(12, y + 9);
    reset.value.setPosition(228, y + 9);
    refreshRows();
    Screen::onEnter(tft);
}

void SettingsScreen::step(int8_t delta)
{
    do
    {
        selected = (selected + ROW_COUNT + delta) % ROW_COUNT;
    } while (selected == ROW_MENU && !menuShown);
    refreshRows();
    sound.playMenu();
}

void SettingsScreen::onUpPress()
{
    step(-1);
}

void SettingsScreen::onDownPress()
{
    step(1);
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
        refreshRows();
        screenManager.repaint();
        sound.playMenu();
        return;
    case ROW_MENU:
        sound.playMenu();
        toMenu();
        return;
    case ROW_RESET:
    {
        const char *lines[DialogScreen::LINES] = {"Apagar todo o progresso?", "Bosses, itens, XP e NaN", "serao perdidos.",
                                                  ""};
        sound.playNotice();
        dialog.open("Resetar jogo", DialogIcon::WARNING, lines, "Sim", "Nao", reset);
        return;
    }
    }
    changed();
    refreshRows();
    sound.playMenu();
}
