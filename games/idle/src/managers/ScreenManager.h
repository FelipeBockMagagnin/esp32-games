#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "../screens/Screen.h"

class ScreenManager
{
private:
    TFT_eSPI &tft;
    std::vector<Screen *> screens;
    std::vector<const char *> navNames; // What the neighbours' header labels call each screen
    std::vector<bool> locked;
    int currentIndex;
    Screen *modal; // Shown over the rotation, outside it; takes every input while open

    // A screen change sweeps the old body away before the new screen paints. currentIndex
    // already names the new screen meanwhile, so a second press skips on without painting it.
    int8_t sweepDirection; // 0 when no sweep runs
    unsigned long sweepStart;
    int16_t sweptCols;

    int stepFrom(int index, int direction) const; // Next unlocked screen that way, or index itself
    void refreshNavLabels();
    void startScreen(int index, int direction);
    void advanceSweep(unsigned long now);
    void finishSweep();

public:
    ScreenManager(TFT_eSPI &display);

    // navName labels this screen in its neighbours' headers. A locked screen is skipped by
    // navigation and left out of those labels until it is unlocked.
    void addScreen(Screen *screen, const char *navName, bool startLocked = false);
    void setLocked(Screen *screen, bool isLocked);
    void setScreen(int index);
    void nextScreen();
    void previousScreen();

    // A modal screen replaces the current one until closed, and screen navigation stops
    // meanwhile. Closing re-enters the screen underneath, which repaints it whole.
    void openModal(Screen *screen);
    void closeModal();
    // Swaps the open modal for another without showing the screen underneath in between
    void replaceModal(Screen *screen);
    bool hasModal() const { return modal != nullptr; }
    void repaint();

    Screen *getCurrentScreen() const;
    int getCurrentIndex() const;
    int getScreenCount() const;

    void update(unsigned long now);

    void handleConfirmPress();
    void handleUpPress();
    void handleDownPress();
    void handleSelectPress() { handleDownPress(); }
    bool handleBackPress();
};

#endif // SCREEN_MANAGER_H
