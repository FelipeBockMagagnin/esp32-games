#include "GameOverScreen.h"

static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 40;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 104;
static const int16_t CENTER_X = 120;
static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 160;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 6;
static const int16_t HINT_Y = 270;

GameOverScreen::GameOverScreen(const Round &round, const Profile &profile, PlayScreen &play,
                               ScreenManager &screenManager, SoundManager &sound, bool (*pay)(uint32_t price))
    : header("Game over"),
      card(CARD_X, CARD_Y, CARD_W, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      pointsText(CENTER_X, CARD_Y + 10, "", Theme::TEXT, 3, TC_DATUM),
      bestText(CENTER_X, CARD_Y + 40, "", Theme::TEXT_2, 1, TC_DATUM),
      earnedText(CENTER_X, CARD_Y + 58, "", Theme::GOLD, 2, TC_DATUM),
      totalText(CENTER_X, CARD_Y + 82, "", Theme::TEXT_2, 1, TC_DATUM),
      hintText(CENTER_X, HINT_Y, "Up/Down choose - Confirm OK", Theme::TEXT_DIM, 1, TC_DATUM),
      round(round),
      profile(profile),
      play(play),
      screenManager(screenManager),
      sound(sound),
      pay(pay)
{
    addElement(&header);
    setHeader(&header);
    addElement(&card);
    addElement(&pointsText, Theme::CARD);
    addElement(&bestText, Theme::CARD);
    addElement(&earnedText, Theme::CARD);
    addElement(&totalText, Theme::CARD);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    rows[ROW_AGAIN].setTitle("Play again");
    rows[ROW_AGAIN].setSubtitle("A new round from zero");
    rows[ROW_TITLE].setTitle("Title");
    rows[ROW_TITLE].setSubtitle("Shop and settings");
    addElement(&hintText);
}

bool GameOverScreen::canContinue() const
{
    return !round.hasContinued() && profile.coins >= round.getContinuePrice();
}

void GameOverScreen::refreshRows()
{
    char buf[32];
    rows[ROW_CONTINUE].setTitle("Continue");
    if (round.hasContinued())
    {
        rows[ROW_CONTINUE].setSubtitle("Only once a round");
        rows[ROW_CONTINUE].setValue("");
        rows[ROW_CONTINUE].setShowCoin(false);
    }
    else
    {
        rows[ROW_CONTINUE].setSubtitle("Keep the points, same pace");
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)round.getContinuePrice());
        rows[ROW_CONTINUE].setValue(buf);
        rows[ROW_CONTINUE].setShowCoin(true);
        rows[ROW_CONTINUE].setValueColor(canContinue() ? Theme::GOLD : Theme::BAD);
    }
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        bool dimmed = i == ROW_CONTINUE && !canContinue();
        rows[i].setState(i == selected ? ListRow::SELECTED : (dimmed ? ListRow::DIMMED : ListRow::NORMAL));
    }
}

void GameOverScreen::onEnter(TFT_eSPI &tft)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)round.getPoints());
    pointsText.setText(buf);
    if (newBest)
    {
        bestText.setText("A new best!");
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
    snprintf(buf, sizeof(buf), "%lu in all - x%lu reached", (unsigned long)profile.coins,
             (unsigned long)round.getMultiplier());
    totalText.setText(buf);
    selected = canContinue() ? ROW_CONTINUE : ROW_AGAIN;
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
    switch (selected)
    {
    case ROW_CONTINUE:
        if (!canContinue() || !pay(round.getContinuePrice()))
        {
            sound.playError();
            return;
        }
        screenManager.closeModal();
        play.continueRound();
        break;
    case ROW_AGAIN:
        screenManager.closeModal();
        play.newRound();
        break;
    default:
        sound.playMenu();
        play.toTitle();
        screenManager.closeModal();
        break;
    }
}

bool GameOverScreen::onBackPress()
{
    sound.playMenu();
    play.toTitle();
    screenManager.closeModal();
    return true;
}
