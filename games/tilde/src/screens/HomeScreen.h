#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "Screen.h"
#include "Game.h"
#include "../art/PetView.h"
#include "../art/Spring.h"

// The room with the pet in it, the needs under it, a bar of care buttons, and now and then
// what the pet is up to in a speech bubble
class HomeScreen : public Screen
{
public:
    // `hatchScreen` opens once the hatching has played; `openPlay` goes to the minigames
    HomeScreen(Display &display, Game &game, Screen &hatchScreen, void (*openPlay)());

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

    // A line for the bubble, with the pet's name for "%s"
    void say(const char *line, unsigned long now, unsigned long ms = 4000);
    // After the pet changed under it (a new egg, a dev command): start the view over
    void restart(unsigned long now);

protected:
    const char *title() const override { return titleText; }
    void drawScene(Canvas &c) const override;
    uint8_t segments(int16_t *edges) const override;
    uint32_t bandKey(uint8_t segment) const override;

private:
    enum Item : uint8_t
    {
        MEAL,
        SNACK,
        PLAY,
        CLEAN,
        CUDDLE,
        LAMP,
        MEDS,
        ITEM_COUNT
    };

    Game &game;
    Screen &hatchScreen;
    void (*openPlay)();
    PetView view;
    uint8_t selected = 0;
    FormId seenForm = FORM_EGG;
    bool hatchPending = false;
    char message[48] = "";
    unsigned long messageAt = 0;
    unsigned long messageUntil = 0;
    unsigned long nextFlavor = 0;
    char titleText[NAME_LEN + 8] = "";
    float shownNeeds[4] = {};
    Spring pop = Spring(320, 16, 1); // the selected button's
    unsigned long lastUpdate = 0;

    uint8_t itemCount() const;
    bool available(uint8_t item) const;
    float buttonX(uint8_t item) const;
    void select(uint8_t item, unsigned long now);
    void doItem(uint8_t item, unsigned long now);
    void drawNeeds(Canvas &c) const;
    void drawBar(Canvas &c) const;
    void drawBubble(Canvas &c, unsigned long now) const;
};

#endif // HOME_SCREEN_H
