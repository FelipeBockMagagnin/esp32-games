#include "ScreenManager.h"

ScreenManager::ScreenManager(TFT_eSPI &display)
    : tft(display), currentIndex(-1), modal(nullptr)
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
    if (modal != nullptr || index < 0 || index >= (int)screens.size() || index == currentIndex)
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

void ScreenManager::openModal(Screen *screen)
{
    if (screen == nullptr || modal != nullptr)
    {
        return;
    }

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
