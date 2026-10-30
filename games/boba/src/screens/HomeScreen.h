#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../pet/Pet.h"
#include "../ui/Panel.h"

// The toy itself: the pet on the matrix, the icons round it, and every small scene the original
// plays between them. All the care actions go through here.
class HomeScreen : public Screen
{
public:
    HomeScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
               SoundManager &sound, Screen &gameScreen);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

    // Whether the arrows and the hold gestures are free: nothing is playing or open
    bool isIdle() const { return mode == Mode::PET || mode == Mode::GRAVE; }
    void handleEvents(uint16_t events);

private:
    enum class Mode : uint8_t
    {
        PET,
        FOOD_MENU,
        LIGHT_MENU,
        METER,
        CLOCK,
        SCENE,
        GRAVE
    };

    enum class Scene : uint8_t
    {
        NONE,
        EAT_MEAL,
        EAT_SNACK,
        REFUSE,
        CHEER,
        FLUSH,
        INJECT,
        SCOLD,
        HATCH,
        EVOLVE,
        DEATH
    };

    Pet &pet;
    LcdMatrix &lcd;
    Panel &panel;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &gameScreen;

    Mode mode = Mode::PET;
    Scene scene = Scene::NONE;
    uint8_t sceneStep = 0;
    int8_t selected = Panel::NONE;
    uint8_t menuRow = 0;
    uint8_t meterPage = 0;
    uint8_t graveStep = 0;
    uint8_t flushedPoops = 0;
    int16_t petX = 8;
    bool facingLeft = false;
    bool second = false;
    FormId shownForm = FORM_EGG;
    unsigned long nextStepAt = 0;
    unsigned long lastInputAt = 0;
    unsigned long clockEndsAt = 0;

    void step(unsigned long now);
    void stepWander();
    void stepScene();
    void startScene(Scene next);
    void endScene();
    void act(Action action, Scene success);
    void confirmIcon();
    void moveSelection(int8_t delta);
    void back();
    void input();

    void draw();
    void drawPet();
    void drawPetAt(int16_t x, Pose pose);
    void drawPoop(uint8_t index);
    void drawExtras();
    void drawMenu(const char *first, const char *second);
    void drawMeter();
    void drawClock();
    void drawScene();
    void drawGrave();
};

#endif // HOME_SCREEN_H
