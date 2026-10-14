#ifndef CONFIRM_SCREEN_H
#define CONFIRM_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/ListRow.h"
#include "../ui/ScreenWipe.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// A Yes/No question opened as a modal before something that cannot be undone and restarts
// the board: resetting the game, ascending. It defaults to No, so reaching it can never
// do anything by itself.
class ConfirmScreen : public Screen
{
public:
    struct Texts
    {
        const char *title;
        const char *question;    // Size 2, up to 17 chars
        const char *warning;     // Size 1, up to 36 chars, in the accent colour
        const char *note;        // Size 1, up to 36 chars, under the warning
        const char *yesSubtitle;
        const char *busy;        // Shown once Yes is confirmed, while the action runs
    };

    // The action runs after the busy text reaches the panel, or with `flashOut` after a burst
    // of light in the accent has covered it, and does not return
    ConfirmScreen(const Texts &texts, uint16_t accent, void (*action)(), ScreenManager &screenManager,
                  SoundManager &sound, bool flashOut = false);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    Header header;

    Box warningCard;
    Text questionText;
    Text warningText;
    Text noteText;

    ListRow noRow;
    ListRow yesRow;

    Text hintText;
    Text busyText;
    ScreenWipe wipe;

    void (*action)();
    ScreenManager &screenManager;
    SoundManager &sound;
    bool yesSelected;
    bool busy;
    bool flashOut;

    void refreshRows();
};

#endif // CONFIRM_SCREEN_H
