#ifndef BUILD_MENU_SCREEN_H
#define BUILD_MENU_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/ListRow.h"
#include "../game/World.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// A modal over the habitat: what can be done to the tile under the cursor (build on grass,
// plant on soil, clear anything built back to grass)
class BuildMenuScreen : public Screen
{
public:
    BuildMenuScreen(World &world, ScreenManager &screenManager, SoundManager &sound);

    void openFor(uint16_t tile);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    enum class Action : uint8_t
    {
        BUILD,
        PLANT,
        DEMOLISH
    };
    struct Option
    {
        Action action;
        uint8_t index; // BuildId or crop
    };
    static const uint8_t MAX_OPTIONS = BUILD_COUNT > CROP_COUNT + 1 ? BUILD_COUNT : CROP_COUNT + 1;

    Header header;
    ListRow rows[MAX_OPTIONS];
    Text hintText;

    World &world;
    ScreenManager &screenManager;
    SoundManager &sound;
    uint16_t tile;
    Option options[MAX_OPTIONS];
    uint8_t optionCount;
    uint8_t selected;

    void buildOptions();
    uint32_t priceOf(const Option &option) const;
    void refreshRows();
};

#endif // BUILD_MENU_SCREEN_H
