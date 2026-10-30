#include "DialogScreen.h"
#include "../ui/Themes.h"

static const int16_t BOX_W = 226;
static const int16_t BOX_H = 132;
static const int16_t BOX_X = (DISPLAY_W - BOX_W) / 2;
static const int16_t BOX_Y = (DISPLAY_H - Win::TASKBAR_H - BOX_H) / 2;
static const int16_t CAPTION_H = 18;
static const int16_t BUTTON_W = 64;
static const int16_t BUTTON_H = 22;
static const int16_t BUTTON_Y = BOX_Y + BOX_H - BUTTON_H - 8;

DialogScreen::Box::Box() : UI(BOX_X, BOX_Y, BOX_W, BOX_H), icon(DialogIcon::INFO)
{
    title[0] = '\0';
    memset(lines, 0, sizeof(lines));
}

void DialogScreen::Box::draw(TFT_eSPI &tft)
{
    WinStyle s = activeTheme().style;
    // A soft shadow on the newer styles
    if (s != WinStyle::CLASSIC)
    {
        tft.fillRect(x + 4, y + 4, w, h, tft.alphaBlend(110, Theme::EDGE_DARK, Theme::DESKTOP));
    }
    tft.fillRect(x, y, w, h, Theme::FACE);
    if (s == WinStyle::CLASSIC)
    {
        Win::bevel(tft, x, y, w, h, true);
        Win::gradientH(tft, x + 3, y + 3, w - 6, CAPTION_H - 2, Theme::TITLE_A, Theme::TITLE_B);
    }
    else
    {
        tft.drawRect(x, y, w, h, s == WinStyle::LUNA ? Theme::TITLE_B : Theme::EDGE_DARK);
        Win::gradientV(tft, x + 1, y + 1, w - 2, CAPTION_H, Theme::TITLE_A, Theme::TITLE_B);
    }
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(Theme::TITLE_TEXT);
    tft.drawString(title, x + 7, y + 7);
    tft.drawString(title, x + 8, y + 7);

    int16_t cx = x + 26;
    int16_t cy = y + CAPTION_H + 26;
    const char *glyph = "i";
    switch (icon)
    {
    case DialogIcon::INFO:
        tft.fillCircle(cx, cy, 13, Theme::SELECT);
        tft.drawCircle(cx, cy, 13, Theme::EDGE_DARK);
        break;
    case DialogIcon::QUESTION:
        tft.fillCircle(cx, cy, 13, Theme::SELECT);
        tft.drawCircle(cx, cy, 13, Theme::EDGE_DARK);
        glyph = "?";
        break;
    case DialogIcon::WARNING:
        tft.fillTriangle(cx, cy - 14, cx - 14, cy + 11, cx + 14, cy + 11, Theme::GOLD);
        tft.drawTriangle(cx, cy - 14, cx - 14, cy + 11, cx + 14, cy + 11, Theme::EDGE_DARK);
        glyph = "!";
        cy += 2;
        break;
    }
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::FIELD);
    tft.drawString(glyph, cx + 1, cy + 1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(Theme::TEXT);
    for (uint8_t i = 0; i < LINES; i++)
    {
        tft.drawString(lines[i], x + 50, y + CAPTION_H + 12 + i * 12);
    }
}

DialogScreen::DialogScreen(ScreenManager &screenManager, SoundManager &sound)
    : Screen(nullptr, Theme::DESKTOP),
      screenManager(screenManager),
      sound(sound),
      yesButton(0, BUTTON_Y, BUTTON_W, BUTTON_H),
      noButton(0, BUTTON_Y, BUTTON_W, BUTTON_H)
{
    addElement(&box);
    addElement(&yesButton, Theme::FACE);
    addElement(&noButton, Theme::FACE);
    addTaskbar();
}

void DialogScreen::open(const char *title, DialogIcon icon, const char *const lines[LINES], const char *yes,
                        const char *no, void (*newAction)())
{
    strncpy(box.title, title, sizeof(box.title) - 1);
    box.title[sizeof(box.title) - 1] = '\0';
    box.icon = icon;
    for (uint8_t i = 0; i < LINES; i++)
    {
        strncpy(box.lines[i], lines[i] != nullptr ? lines[i] : "", LINE_CHARS);
        box.lines[i][LINE_CHARS] = '\0';
    }
    twoButtons = no != nullptr;
    yesButton.setLabel(yes);
    if (twoButtons)
    {
        noButton.setLabel(no);
        yesButton.setPosition(DISPLAY_W / 2 - BUTTON_W - 4, BUTTON_Y);
        noButton.setPosition(DISPLAY_W / 2 + 4, BUTTON_Y);
    }
    else
    {
        yesButton.setPosition((DISPLAY_W - BUTTON_W) / 2, BUTTON_Y);
    }
    noButton.setVisible(twoButtons);
    action = newAction;
    yesChosen = !twoButtons;
    if (screenManager.hasModal())
    {
        screenManager.replaceModal(this);
    }
    else
    {
        screenManager.openModal(this);
    }
}

void DialogScreen::onEnter(TFT_eSPI &tft)
{
    yesButton.setFocused(yesChosen);
    noButton.setFocused(!yesChosen);
    Screen::onEnter(tft);
}

void DialogScreen::choose(bool yes)
{
    if (!twoButtons || yes == yesChosen)
    {
        return;
    }
    yesChosen = yes;
    yesButton.setFocused(yesChosen);
    noButton.setFocused(!yesChosen);
    sound.playMenu();
}

bool DialogScreen::onBackPress()
{
    choose(true);
    return true;
}

bool DialogScreen::onRightPress()
{
    choose(false);
    return true;
}

void DialogScreen::onConfirmPress()
{
    sound.playMenu();
    void (*run)() = yesChosen ? action : nullptr;
    screenManager.closeModal();
    if (run != nullptr)
    {
        run();
    }
}
