#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../crawl/Save.h"
#include "../ui/ArtImage.h"

// The only screen in the rotation: the name, the lich, Continue when a run is saved, New run,
// Settings, and the records
class TitleScreen : public Screen
{
public:
    struct Hooks
    {
        void (*continueRun)();
        void (*newRun)();
        void (*settings)();
    };

    TitleScreen(SoundManager &sound, const SaveBlob &save, const Hooks &hooks);

    void onEnter(TFT_eSPI &tft) override;
    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override { return true; }
    bool onRightPress() override { return true; }

private:
    static const uint8_t MENU_ROWS = 3;

    enum Choice : uint8_t
    {
        CONTINUE,
        NEW_RUN,
        SETTINGS
    };

    SoundManager &sound;
    const SaveBlob &save;
    Hooks hooks;
    uint8_t selected = 0;

    Text titleText;
    Text taglineText;
    Text menuRows[MENU_ROWS];
    Text recordText;
    Text hintText;
    ArtImage lich;

    bool hasRun() const { return save.run.floor != 0; }
    void step(int8_t delta);
    void refreshMenu();
};

#endif // TITLE_SCREEN_H
