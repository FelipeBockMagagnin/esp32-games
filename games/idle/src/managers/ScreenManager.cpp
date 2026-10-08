#include "ScreenManager.h"

ScreenManager::ScreenManager(TFT_eSPI &display)
    : tft(display), currentIndex(-1)
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
    Screen *screen = getCurrentScreen();
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
    if (index < 0 || index >= (int)screens.size() || index == currentIndex)
    {
        return;
    }

    if (currentIndex >= 0 && currentIndex < (int)screens.size())
    {
        screens[currentIndex]->onExit();
    }

    currentIndex = index;
    // Labels first, so the header's first draw after the switch already has them
    refreshNavLabels();
    screens[currentIndex]->onEnter(tft);
}

void ScreenManager::nextScreen()
{
    if (screens.empty())
    {
        return;
    }

    setScreen(stepFrom(currentIndex, 1));
}

void ScreenManager::previousScreen()
{
    if (screens.empty())
    {
        return;
    }

    setScreen(stepFrom(currentIndex, -1));
}

Screen *ScreenManager::getCurrentScreen() const
{
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
    if (currentIndex >= 0 && currentIndex < (int)screens.size())
    {
        screens[currentIndex]->update(now);
        screens[currentIndex]->render(tft);
    }
}

void ScreenManager::handleConfirmPress()
{
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onConfirmPress();
    }
}

void ScreenManager::handleUpPress()
{
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onUpPress();
    }
}

void ScreenManager::handleDownPress()
{
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onDownPress();
    }
}

bool ScreenManager::handleBackPress()
{
    // The current screen gets first refusal (e.g. inventory sub-list or combat smite)
    Screen *screen = getCurrentScreen();
    if (screen != nullptr && screen->onBackPress())
    {
        return true;
    }
    previousScreen();
    return false;
}
