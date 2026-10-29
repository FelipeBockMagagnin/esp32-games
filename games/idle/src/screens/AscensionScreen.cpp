#include "AscensionScreen.h"

static const unsigned long COUNT_MS = 1600;
static const unsigned long TOTAL_AT_MS = 1900;
static const unsigned long END_MS = 4200;
// Pressed after the count, the restart still waits for the busy text to be on screen
static const unsigned long SKIP_TO_MS = END_MS - 300;

static const int16_t CENTER_X = 120;

AscensionScreen::AscensionScreen(const GameState &game, const Ascension &ascension, SoundManager &sound,
                                 void (*done)())
    : title(CENTER_X, 34, "ASCENSION", Theme::EPIC, 3, TC_DATUM),
      runText(CENTER_X, 66, "", Theme::TEXT_2, 1, TC_DATUM),
      // Opaque, so the count repaints without a flicker
      gainText(CENTER_X, 96, "+0", Theme::GOLD, 4, TC_DATUM, false, Theme::BG),
      gainLabel(CENTER_X, 132, "points earned", Theme::TEXT_2, 1, TC_DATUM),
      totalText(CENTER_X, 152, "", Theme::TEXT, 2, TC_DATUM),
      busyText(CENTER_X, 302, "Restarting...", Theme::TEXT_DIM, 1, TC_DATUM),
      stars(0, 176, 240, 120),
      game(game),
      ascension(ascension),
      sound(sound),
      done(done),
      gained(0),
      shownGain(0),
      start(0),
      finishing(false)
{
    addElement(&title);
    addElement(&runText);
    addElement(&gainText);
    addElement(&gainLabel);
    addElement(&totalText);
    addElement(&busyText);
    addElement(&stars);
}

void AscensionScreen::onEnter(TFT_eSPI &tft)
{
    gained = ascension.getPendingPoints(game.getTotalGoldEarned());
    shownGain = 0;
    start = 0;
    finishing = false;

    char buf[32];
    snprintf(buf, sizeof(buf), "Run %u complete", (unsigned)ascension.getAscensions() + 1);
    runText.setText(buf);
    gainText.setText("+0");
    snprintf(buf, sizeof(buf), "Total %lu points", (unsigned long)(ascension.getPointsEarned() + gained));
    totalText.setText(buf);
    totalText.setVisible(false);
    busyText.setVisible(false);
    Screen::onEnter(tft);
}

void AscensionScreen::update(unsigned long now)
{
    if (start == 0)
    {
        start = now;
        stars.start(now);
        sound.playAscend();
    }
    stars.update(now);
    unsigned long age = now - start;

    // Eased out: the count races at first and settles on the total
    int32_t t = age < COUNT_MS ? (int32_t)(age * 256 / COUNT_MS) : 256;
    t = 256 - (256 - t) * (256 - t) / 256;
    uint32_t count = (uint32_t)((uint64_t)gained * t / 256);
    if (count != shownGain)
    {
        shownGain = count;
        char buf[16];
        snprintf(buf, sizeof(buf), "+%lu", (unsigned long)count);
        gainText.setText(buf);
    }

    totalText.setVisible(age >= TOTAL_AT_MS);
    busyText.setVisible(age >= SKIP_TO_MS);
    finishing = age >= END_MS;
}

void AscensionScreen::render(TFT_eSPI &tft)
{
    Screen::render(tft);
    if (finishing)
    {
        done();
    }
}

void AscensionScreen::skip()
{
    if (start == 0)
    {
        return;
    }
    unsigned long age = millis() - start;
    unsigned long target = age < COUNT_MS ? TOTAL_AT_MS : SKIP_TO_MS;
    if (age < target)
    {
        start -= target - age;
    }
}
