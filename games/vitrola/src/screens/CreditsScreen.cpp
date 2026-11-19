#include "CreditsScreen.h"

static const char *const LINES[] = {
    "#System Beeps",
    "Album de PC speaker de Shiru",
    "(2017-2019), CC-BY 3.0.",
    "shiru.untergrund.net",
    "",
    "#Classicos e cantigas",
    "Melodias em dominio publico:",
    "Beethoven, Bach, Mozart, Grieg,",
    "Pachelbel, Vivaldi, Strauss II,",
    "Tchaikovsky, Rimsky-Korsakov,",
    "Offenbach, Joplin, Petzold,",
    "Brahms, Foster, Pierpont e o",
    "folclore. Arranjos para 1 bit",
    "feitos para esta vitrola.",
    "",
    "OK ou < volta",
};

CreditsScreen::CreditsScreen(ScreenManager &screenManager, Screen &settings)
    : header("Creditos"), screenManager(screenManager), settings(settings)
{
    addElement(&header);
    setHeader(&header);
}

void CreditsScreen::onEnter(TFT_eSPI &tft)
{
    Screen::onEnter(tft);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    int16_t y = Header::HEIGHT + 12;
    for (const char *line : LINES)
    {
        bool heading = line[0] == '#';
        tft.setTextColor(heading ? Theme::SELECT : Theme::TEXT);
        tft.drawString(heading ? line + 1 : line, 14, y);
        y += heading ? 14 : 12;
    }
}

void CreditsScreen::onConfirmPress()
{
    screenManager.replaceModal(&settings);
}

bool CreditsScreen::onBackPress()
{
    screenManager.replaceModal(&settings);
    return true;
}
