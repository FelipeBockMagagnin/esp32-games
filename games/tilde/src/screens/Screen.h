#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../gfx/Display.h"
#include "../gfx/Theme.h"

enum class Key : uint8_t
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    CONFIRM
};

// A screen draws the whole display every frame, from its state and the frame's time. update()
// moves the state on; drawScene() only reads it, since it runs once per band. The header (the
// title between the neighbours' names, or a notice sliding over it) is drawn on top for any
// screen whose title() is not nullptr.
class Screen : public Drawable
{
public:
    static const int16_t HEADER_H = 26;

    explicit Screen(Display &display) : display(display) {}

    virtual void onEnter(TFT_eSPI &tft) { repaint(tft); }
    void repaint(TFT_eSPI &tft);
    virtual void onExit() {}
    virtual void update(unsigned long now) {}
    virtual void render(TFT_eSPI &tft);
    void draw(Canvas &c) const override;

    virtual void onConfirmPress() {}
    // Sent at a steady pace while a button stays held; false stops the repeat until the next press
    virtual bool onRepeat(Key key) { return false; }
    virtual void onUpPress() {}
    virtual void onDownPress() {}
    // Return true to consume Left or Right. Screens that do not override them fall through to
    // ScreenManager, which steps through the rotation.
    virtual bool onBackPress() { return false; }
    virtual bool onRightPress() { return false; }
    // False while the held buttons mean something here (a minigame), so holding Left does
    // not open Settings
    virtual bool allowsGestures() const { return true; }

    void setNavLabels(const char *back, const char *next)
    {
        backLabel = back;
        nextLabel = next;
    }

    static void notify(const char *text, uint8_t accent, unsigned long now, unsigned long ms = 3000);

protected:
    virtual const char *title() const { return nullptr; }
    virtual void drawScene(Canvas &c) const = 0;
    void forceFrame() { frameForced = true; }
    // The header segment's key (see Drawable::bandKey): its labels, and a notice while it moves
    uint32_t headerKey() const;
    static uint32_t keyMix(uint32_t key, uint32_t value) { return ((key ^ value) * 16777619u) | 1; }
    static uint32_t keyText(uint32_t key, const char *text);
    // The time of the frame being drawn: what drawScene() animates by
    unsigned long frameTime() const { return frameNow; }

    Display &display;
    unsigned long frameMs = 33;

private:
    void drawHeader(Canvas &c) const;

    const char *backLabel = "";
    const char *nextLabel = "";
    unsigned long lastFrame = 0;
    unsigned long frameNow = 0;
    bool frameForced = true;

    static char noticeText[48];
    static uint8_t noticeAccent;
    static unsigned long noticeAt;
    static unsigned long noticeUntil;
};

#endif // SCREEN_H
