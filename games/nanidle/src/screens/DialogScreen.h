#ifndef DIALOG_SCREEN_H
#define DIALOG_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

enum class DialogIcon : uint8_t
{
    INFO,
    WARNING,
    QUESTION
};

// A message box over the desktop, opened as a modal: up to three lines beside an icon, and
// OK alone or Yes/No. A question defaults to No, so reaching it never does anything by itself.
class DialogScreen : public Screen
{
public:
    static const uint8_t LINES = 4;
    static const uint8_t LINE_CHARS = 26;

    DialogScreen(ScreenManager &screenManager, SoundManager &sound);

    // `no` nullptr shows OK alone. `action` runs after the box closes, on OK or Yes.
    void open(const char *title, DialogIcon icon, const char *const lines[LINES], const char *yes,
              const char *no, void (*action)());

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override { choose(!yesChosen); }
    void onDownPress() override { choose(!yesChosen); }
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;

private:
    class Box : public UI
    {
    public:
        Box();
        void draw(TFT_eSPI &tft) override;
        char title[24];
        DialogIcon icon;
        char lines[LINES][LINE_CHARS + 1];
    };

    ScreenManager &screenManager;
    SoundManager &sound;
    Box box;
    WinButton yesButton;
    WinButton noButton;
    bool twoButtons = false;
    bool yesChosen = true;
    void (*action)() = nullptr;

    void choose(bool yes);
};

#endif // DIALOG_SCREEN_H
