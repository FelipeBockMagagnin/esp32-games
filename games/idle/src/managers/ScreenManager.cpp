#include "ScreenManager.h"
#include "../ui/Header.h"

static const unsigned long SWEEP_MS = 110;
static const int16_t SWEEP_EDGE = 3;

ScreenManager::ScreenManager(TFT_eSPI &display)
    : tft(display), currentIndex(-1), modal(nullptr), sweepDirection(0), sweepStart(0), sweptCols(0)
{
}

void ScreenManager::addScreen(Screen *screen, const char *navName, bool startLocked)
{
    if (screen == nullptr)
    {
        return;
    }

    screens.push_back(screen);
    navNames.push_back(navName);
    locked.push_back(startLocked);

    // The first unlocked screen added becomes the current one
    if (currentIndex == -1 && !startLocked)
    {
        setScreen((int)screens.size() - 1);
    }
    else
    {
        refreshNavLabels();
    }
}

void ScreenManager::setLocked(Screen *screen, bool isLocked)
{
    for (size_t i = 0; i < screens.size(); i++)
    {
        if (screens[i] == screen && locked[i] != isLocked)
        {
            locked[i] = isLocked;
            refreshNavLabels();
        }
    }
}

int ScreenManager::stepFrom(int index, int direction) const
{
    int count = (int)screens.size();
    for (int step = 1; step < count; step++)
    {
        int candidate = ((index + direction * step) % count + count) % count;
        if (!locked[candidate])
        {
            return candidate;
        }
    }
    return index;
}

void ScreenManager::refreshNavLabels()
{
    if (currentIndex < 0 || currentIndex >= (int)screens.size())
    {
        return;
    }
    Screen *screen = screens[currentIndex];
    if (screen == nullptr)
    {
        return;
    }

    // With a single screen unlocked both labels stay empty, which hides the arrows too
    int back = stepFrom(currentIndex, -1);
    int next = stepFrom(currentIndex, 1);
    screen->setNavLabels(back == currentIndex ? "" : navNames[back],
                         next == currentIndex ? "" : navNames[next]);
}

void ScreenManager::setScreen(int index)
{
    startScreen(index, 0);
}

void ScreenManager::startScreen(int index, int direction)
{
    if (modal != nullptr || index < 0 || index >= (int)screens.size() || index == currentIndex)
    {
        return;
    }

    bool hadScreen = currentIndex >= 0 && currentIndex < (int)screens.size();
    if (hadScreen)
    {
        screens[currentIndex]->onExit();
    }

    currentIndex = index;
    // Labels first, so the header's first draw after the switch already has them
    refreshNavLabels();
    if (!hadScreen || direction == 0)
    {
        sweepDirection = 0;
        screens[currentIndex]->onEnter(tft);
        return;
    }
    if (sweepDirection == 0)
    {
        sweepStart = millis();
        sweptCols = 0;
    }
    sweepDirection = (int8_t)direction;
}

// The body clears from the side the player is heading to, with a lit leading edge
void ScreenManager::advanceSweep(unsigned long now)
{
    unsigned long age = now - sweepStart;
    int16_t target = age >= SWEEP_MS ? TFT_WIDTH : (int16_t)(TFT_WIDTH * (int32_t)age / SWEEP_MS);
    int16_t top = Header::HEIGHT;
    int16_t height = TFT_HEIGHT - top;
    uint16_t bg = screens[currentIndex]->getBgColor();
    if (target <= sweptCols)
    {
        return;
    }
    if (target >= TFT_WIDTH)
    {
        finishSweep();
        return;
    }
    int16_t x = sweepDirection > 0 ? TFT_WIDTH - target : sweptCols;
    tft.fillRect(x, top, target - sweptCols, height, bg);
    sweptCols = target;
    int16_t edge = sweepDirection > 0 ? TFT_WIDTH - sweptCols - SWEEP_EDGE : sweptCols;
    tft.fillRect(edge, top, SWEEP_EDGE, height, Theme::SELECT);
}

void ScreenManager::finishSweep()
{
    if (sweepDirection == 0)
    {
        return;
    }
    sweepDirection = 0;
    screens[currentIndex]->onEnter(tft);
}

void ScreenManager::nextScreen()
{
    if (screens.empty())
    {
        return;
    }

    startScreen(stepFrom(currentIndex, 1), 1);
}

void ScreenManager::previousScreen()
{
    if (screens.empty())
    {
        return;
    }

    startScreen(stepFrom(currentIndex, -1), -1);
}

void ScreenManager::openModal(Screen *screen)
{
    if (screen == nullptr || modal != nullptr)
    {
        return;
    }
    // Unpainted, the screen under the modal would come back half swept when it closes
    sweepDirection = 0;

    if (currentIndex >= 0 && currentIndex < (int)screens.size())
    {
        screens[currentIndex]->onExit();
    }
    modal = screen;
    modal->setNavLabels("", "");
    modal->onEnter(tft);
}

void ScreenManager::closeModal()
{
    if (modal == nullptr)
    {
        return;
    }

    modal->onExit();
    modal = nullptr;
    if (currentIndex >= 0 && currentIndex < (int)screens.size())
    {
        screens[currentIndex]->onEnter(tft);
    }
}

void ScreenManager::replaceModal(Screen *screen)
{
    if (modal == nullptr || screen == nullptr)
    {
        openModal(screen);
        return;
    }
    modal->onExit();
    modal = screen;
    modal->setNavLabels("", "");
    modal->onEnter(tft);
}

void ScreenManager::repaint()
{
    finishSweep();
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->repaint(tft);
    }
}

Screen *ScreenManager::getCurrentScreen() const
{
    if (modal != nullptr)
    {
        return modal;
    }
    if (currentIndex >= 0 && currentIndex < (int)screens.size())
    {
        return screens[currentIndex];
    }
    return nullptr;
}

int ScreenManager::getCurrentIndex() const
{
    return currentIndex;
}

int ScreenManager::getScreenCount() const
{
    return (int)screens.size();
}

void ScreenManager::update(unsigned long now)
{
    if (sweepDirection != 0)
    {
        advanceSweep(now);
        return;
    }
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->updateHeader(now);
        screen->update(now);
        screen->render(tft);
    }
}

void ScreenManager::handleConfirmPress()
{
    finishSweep();
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onConfirmPress();
    }
}

bool ScreenManager::handleConfirmRepeat()
{
    Screen *screen = getCurrentScreen();
    return sweepDirection == 0 && screen != nullptr && screen->onConfirmRepeat();
}

void ScreenManager::handleUpPress()
{
    finishSweep();
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onUpPress();
    }
}

void ScreenManager::handleDownPress()
{
    finishSweep();
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onDownPress();
    }
}

bool ScreenManager::handleBackPress()
{
    // Mid-sweep the new screen has not shown yet, so it has no sub-list to back out of
    if (sweepDirection != 0)
    {
        previousScreen();
        return false;
    }
    // The current screen gets first refusal (e.g. inventory sub-list)
    Screen *screen = getCurrentScreen();
    if (screen != nullptr && screen->onBackPress())
    {
        return true;
    }
    // A modal that ignores the back button still keeps the rotation where it is
    if (modal != nullptr)
    {
        return true;
    }
    previousScreen();
    return false;
}
