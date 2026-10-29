#include "GameOverScreen.h"

static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 44;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 112;
static const int16_t CENTER_X = 120;

GameOverScreen::GameOverScreen(const Run &run, const Records &records, SoundManager &sound, void (*newRun)())
    : header("Game over"),
      card(CARD_X, CARD_Y, CARD_W, CARD_H, Theme::BAD, true, Theme::CARD, true, 4),
      floorText(CENTER_X, CARD_Y + 14, "", Theme::TEXT, 2, TC_DATUM),
      killerText(CENTER_X, CARD_Y + 38, "", Theme::BAD, 1, TC_DATUM),
      scoreText(CENTER_X, CARD_Y + 62, "", Theme::GOLD, 2, TC_DATUM),
      bestText(CENTER_X, CARD_Y + 88, "", Theme::TEXT_2, 1, TC_DATUM),
      statsText(CENTER_X, 176, "", Theme::TEXT_2, 1, TC_DATUM),
      hintText(CENTER_X, 220, "Confirm: start a new run", Theme::TEXT_DIM, 1, TC_DATUM),
      run(run),
      records(records),
      sound(sound),
      newRun(newRun),
      newBest(false)
{
    addElement(&header);
    setHeader(&header);
    addElement(&card);
    addElement(&floorText, Theme::CARD);
    addElement(&killerText, Theme::CARD);
    addElement(&scoreText, Theme::CARD);
    addElement(&bestText, Theme::CARD);
    addElement(&statsText);
    addElement(&hintText);
}

void GameOverScreen::onEnter(TFT_eSPI &tft)
{
    char buf[40];
    snprintf(buf, sizeof(buf), "Died on floor %u", (unsigned)run.getFloor());
    floorText.setText(buf);
    if (run.getKiller() < ENEMY_COUNT)
    {
        snprintf(buf, sizeof(buf), "Killed by a %s", ENEMIES[run.getKiller()].name);
        killerText.setText(buf);
    }
    else
    {
        killerText.setText("");
    }
    snprintf(buf, sizeof(buf), "Score %lu", (unsigned long)run.getScore());
    scoreText.setText(buf);
    if (newBest)
    {
        bestText.setText("A new best!");
        bestText.setColor(Theme::GOOD);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Best %lu, deepest floor %u", (unsigned long)records.bestScore,
                 (unsigned)records.bestFloor);
        bestText.setText(buf);
        bestText.setColor(Theme::TEXT_2);
    }
    snprintf(buf, sizeof(buf), "Level %u - %u kills - %lu gold - run %u", (unsigned)run.getLevel(),
             (unsigned)run.getKills(), (unsigned long)run.getGold(), (unsigned)records.runs);
    statsText.setText(buf);
    Screen::onEnter(tft);
}

void GameOverScreen::onConfirmPress()
{
    newRun();
}
