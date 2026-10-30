#include "RebirthScreen.h"
#include "../nan/Format.h"
#include "../ui/Labels.h"

static const char *const INFO[] = {
    "Renascer zera treino, bosses,",
    "ouro, mineradores e aprimora-",
    "mentos. Ficam a XP comprada,",
    "os itens e os pontos NaN: cada",
    "um da +10% em ATQ, DEF e poder.",
};

RebirthScreen::RebirthScreen(Game &game, SoundManager &sound, DialogScreen &dialog, void (*rebirth)())
    : Screen("Renascer"),
      game(game),
      sound(sound),
      dialog(dialog),
      rebirth(rebirth),
      infoPanel(8, 25, 224, 72, false, true),
      button(50, 252, 140, 26)
{
    addElement(&infoPanel);
    for (uint8_t i = 0; i < INFO_LINES; i++)
    {
        info[i] = label(14, 31 + i * 12, TL_DATUM, Theme::TEXT, 1, Theme::FIELD);
        info[i].setText(INFO[i]);
        addElement(&info[i], Theme::FIELD);
    }
    for (uint8_t i = 0; i < STAT_LINES; i++)
    {
        stats[i] = label(12, 112 + i * 22, TL_DATUM, Theme::TEXT, i == 1 ? 2 : 1);
        addElement(&stats[i]);
    }
    button.setLabel("Renascer...");
    button.setFocused(true);
    addElement(&button);
    addTaskbar();
}

void RebirthScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void RebirthScreen::update(unsigned long now)
{
    if (refreshDue(now))
    {
        refresh();
    }
}

void RebirthScreen::refresh()
{
    char a[12], buf[64];
    formatNumber(game.nan(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "Pontos NaN: %s (+%.0f%%)", a, game.rebirthPercent());
    stats[0].setText(buf);
    if (game.canRebirth())
    {
        snprintf(buf, sizeof(buf), "Ganha +%lu NaN", (unsigned long)game.rebirthPoints());
        stats[1].setColor(Theme::GOOD);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Faltam %u bosses", (unsigned)(REBIRTH_MIN_BOSSES - game.bossesBeaten()));
        stats[1].setColor(Theme::TEXT_DIM);
    }
    stats[1].setText(buf);
    snprintf(buf, sizeof(buf), "Bosses nesta vida: %u (min %u)", (unsigned)game.bossesBeaten(),
             (unsigned)REBIRTH_MIN_BOSSES);
    stats[2].setText(buf);
    formatTime(game.runSeconds(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "Tempo nesta vida: %s", a);
    stats[3].setText(buf);
    formatTime(game.playSeconds(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "Renasceu %u vezes, jogo %s", (unsigned)game.rebirths(), a);
    stats[4].setText(buf);
    button.setEnabled(game.canRebirth());
}

void RebirthScreen::onConfirmPress()
{
    if (!game.canRebirth())
    {
        sound.playDenied();
        return;
    }
    char gain[40];
    unsigned long points = game.rebirthPoints();
    snprintf(gain, sizeof(gain), "Ganha %lu ponto%s NaN.", points, points == 1 ? "" : "s");
    const char *lines[DialogScreen::LINES] = {"Renascer agora?", gain, "Treino, bosses e ouro", "voltam a zero."};
    sound.playNotice();
    dialog.open("Renascer", DialogIcon::QUESTION, lines, "Sim", "Nao", rebirth);
}
