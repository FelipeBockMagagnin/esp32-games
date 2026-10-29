#include "GameOverScreen.h"

static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 40;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 92;
static const int16_t CENTER_X = 120;
static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 148;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 6;

GameOverScreen::GameOverScreen(const Round &round, const Profile &profile, PlayScreen &play,
                               ScreenManager &screenManager, SoundManager &sound)
    : header("Game over"),
      card(CARD_X, CARD_Y, CARD_W, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      pointsText(CENTER_X, CARD_Y + 10, "", Theme::TEXT, 4, TC_DATUM),
      bestText(CENTER_X, CARD_Y + 48, "", Theme::TEXT_2, 1, TC_DATUM),
      earnedText(CENTER_X, CARD_Y + 66, "", Theme::GOLD, 2, TC_DATUM),
      round(round),
      profile(profile),
      play(play),
      screenManager(screenManager),
      sound(sound)
{
    addElement(&header);
    setHeader(&header);
    addElement(&card);
    addElement(&pointsText, Theme::CARD);
    addElement(&bestText, Theme::CARD);
    addElement(&earnedText, Theme::CARD);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    rows[ROW_AGAIN].setTitle("Play again");
    rows[ROW_TITLE].setTitle("Title");
}

void GameOverScreen::refreshRows()
{
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void GameOverScreen::onEnter(TFT_eSPI &tft)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)round.getPoints());
    pointsText.setText(buf);
    if (newBest)
    {
        bestText.setText("New best!");
        bestText.setColor(Theme::GOOD);
        sound.playBest();
    }
    else
    {
        snprintf(buf, sizeof(buf), "Best %lu", (unsigned long)profile.best);
        bestText.setText(buf);
        bestText.setColor(Theme::TEXT_2);
    }
    snprintf(buf, sizeof(buf), "+%lu coins", (unsigned long)round.getEarned());
    earnedText.setText(buf);
    selected = ROW_AGAIN;
    refreshRows();
    Screen::onEnter(tft);
}

void GameOverScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : ROW_COUNT - 1;
    refreshRows();
    sound.playMenu();
}

void GameOverScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    refreshRows();
    sound.playMenu();
}

void GameOverScreen::onConfirmPress()
{
    if (selected == ROW_AGAIN)
    {
        screenManager.closeModal();
        play.newRound();
        return;
    }
    onBackPress();
}

bool GameOverScreen::onBackPress()
{
    sound.playMenu();
    play.toTitle();
    screenManager.closeModal();
    return true;
}
