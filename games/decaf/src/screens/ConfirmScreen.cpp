#include "ConfirmScreen.h"

static constexpr Color HINT_COLOR = Theme::TEXT_DIM;

static const int16_t CARD_W = 230;
static const int16_t CARD_X = (DISPLAY_W - CARD_W) / 2;
static const int16_t CARD_Y = 44;
static const int16_t CARD_H = 64;
static const int16_t CARD_CENTER_X = CARD_X + CARD_W / 2;

static const int16_t ROW_X = (DISPLAY_W - ListRow::DEFAULT_W) / 2;
static const int16_t NO_ROW_Y = 124;
static const int16_t YES_ROW_Y = NO_ROW_Y + ListRow::DEFAULT_H + 8;

static const int16_t HINT_Y = 204;
static const int16_t BUSY_Y = 218;

ConfirmScreen::ConfirmScreen(const Texts &texts, Color accent, void (*action)(), ScreenManager &screenManager,
                             SoundManager &sound, bool flashOut)
    : header(texts.title),

      warningCard(CARD_X, CARD_Y, CARD_W, CARD_H, accent, true, Theme::CARD, true, 4),
      questionText(CARD_CENTER_X, CARD_Y + 12, texts.question, Theme::TEXT, 2, TC_DATUM),
      warningText(CARD_CENTER_X, CARD_Y + 36, texts.warning, accent, 1, TC_DATUM),
      noteText(CARD_CENTER_X, CARD_Y + 48, texts.note, Theme::TEXT_2, 1, TC_DATUM),

      noRow(ROW_X, NO_ROW_Y),
      yesRow(ROW_X, YES_ROW_Y),

      hintText(DISPLAY_W / 2, HINT_Y, "Up/Down choose - Confirm OK", HINT_COLOR, 1, TC_DATUM),
      busyText(DISPLAY_W / 2, BUSY_Y, texts.busy, accent, 2, TC_DATUM),
      wipe(accent),

      action(action),
      screenManager(screenManager),
      sound(sound),
      yesSelected(false),
      busy(false),
      flashOut(flashOut)
{
    addElement(&header);
    setHeader(&header);

    addElement(&warningCard);
    addElement(&questionText, Theme::CARD);
    addElement(&warningText, Theme::CARD);
    addElement(&noteText, Theme::CARD);

    noRow.setTitle("No");
    noRow.setSubtitle("Keep playing");
    yesRow.setTitle("Yes");
    yesRow.setSubtitle(texts.yesSubtitle);
    yesRow.setFrameColor(accent);
    yesRow.setTitleColor(accent);
    addElement(&noRow);
    addElement(&yesRow);

    addElement(&hintText);
    busyText.setVisible(false);
    addElement(&busyText);
    addElement(&wipe);

    refreshRows();
}

void ConfirmScreen::refreshRows()
{
    noRow.setState(yesSelected ? ListRow::NORMAL : ListRow::SELECTED);
    yesRow.setState(yesSelected ? ListRow::SELECTED : ListRow::NORMAL);
}

void ConfirmScreen::onEnter(TFT_eSPI &tft)
{
    // Every visit starts on No, whatever was selected when it was last cancelled
    yesSelected = false;
    refreshRows();
    wipe.stop();
    Screen::onEnter(tft);
}

void ConfirmScreen::update(unsigned long now)
{
    wipe.update(now);
}

void ConfirmScreen::render(TFT_eSPI &tft)
{
    Screen::render(tft);

    // Only after the busy text is on the panel, so the player sees why it blanks
    if (busy && (!flashOut || wipe.isDone()))
    {
        action();
    }
}

void ConfirmScreen::onUpPress()
{
    if (busy)
    {
        return;
    }
    yesSelected = !yesSelected;
    refreshRows();
    sound.playMenu();
}

void ConfirmScreen::onDownPress()
{
    onUpPress();
}

void ConfirmScreen::onConfirmPress()
{
    if (busy)
    {
        return;
    }
    if (!yesSelected)
    {
        sound.playMenu();
        screenManager.closeModal();
        return;
    }
    busy = true;
    busyText.setVisible(true);
    if (flashOut)
    {
        wipe.start(millis());
    }
}

bool ConfirmScreen::onBackPress()
{
    if (!busy)
    {
        screenManager.closeModal();
    }
    return true;
}
