#include "ScreenManager.h"

ScreenManager::ScreenManager(TFT_eSPI &display)
    : tft(display), currentIndex(-1)
{
}

void ScreenManager::addScreen(Screen *screen)
{
    if (screen == nullptr)
    {
        return;
    }

    screens.push_back(screen);

    // If this is the first screen added, activate it immediately
    if (currentIndex == -1)
    {
        setScreen(0);
    }
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
    screens[currentIndex]->onEnter(tft);
}

void ScreenManager::nextScreen()
{
    if (screens.empty())
    {
        return;
    }

    int nextIndex = (currentIndex + 1) % screens.size();
    setScreen(nextIndex);
}

void ScreenManager::previousScreen()
{
    if (screens.empty())
    {
        return;
    }

    int prevIndex = (currentIndex - 1 + screens.size()) % screens.size();
    setScreen(prevIndex);
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

void ScreenManager::handleSelectPress()
{
    Screen *screen = getCurrentScreen();
    if (screen != nullptr)
    {
        screen->onSelectPress();
    }
}
