#ifndef UI_WIN_H
#define UI_WIN_H

#include "UI.h"
#include "Theme.h"

// The Windows look, drawn three ways after the active theme's WinStyle: every screen is one
// maximized window over the taskbar.
namespace Win
{
    static const int16_t TITLE_H = 20;
    static const int16_t TASKBAR_H = 20;
    static const int16_t BORDER = 3;
    static const int16_t BODY_X = BORDER;
    static const int16_t BODY_Y = TITLE_H;
    static const int16_t BODY_W = DISPLAY_W - 2 * BORDER;
    static const int16_t BODY_BOTTOM = DISPLAY_H - TASKBAR_H - BORDER;

    void gradientH(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t from, uint16_t to);
    void gradientV(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t from, uint16_t to);
    // A raised button or panel edge, or a sunken one
    void bevel(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, bool raised);
    // A sunken white field, filled
    void field(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h);
    void button(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const char *label, bool focused,
                bool enabled);
    // The title bar with its three caption buttons; the window border around the whole screen
    void window(TFT_eSPI &tft, const char *title);
    // A progress bar inside a sunken track; `filled` is in pixels of the inner width
    void bar(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, int16_t filled, Color fill, Color hi);
    // The selection around a row: a dotted focus rectangle in 98, a highlight frame after
    void focus(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, bool on);
}

// The window: border, title bar and the face behind the body. Registered first on a screen,
// it paints whole only on entering the screen or a theme change.
class WinFrame : public UI
{
public:
    explicit WinFrame(const char *title) : UI(0, 0, DISPLAY_W, DISPLAY_H - Win::TASKBAR_H), title(title) {}
    void draw(TFT_eSPI &tft) override { Win::window(tft, title); }

private:
    const char *title;
};

// The taskbar: the Start button, the neighbouring screens as task buttons, and the tray clock.
// A notice takes the task buttons' place for a while, flashing like a task asking for attention.
class Taskbar : public UI
{
public:
    Taskbar() : UI(0, DISPLAY_H - Win::TASKBAR_H, DISPLAY_W, Win::TASKBAR_H) {}

    void setNav(const char *back, const char *next);
    void setClock(const char *text);
    void setNotice(const char *text, unsigned long until);
    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

private:
    char back[12] = "";
    char next[12] = "";
    char clock[8] = "";
    char notice[24] = "";
    unsigned long noticeUntil = 0;
    bool flash = false;
    unsigned long now = 0;
};

// A progress bar that repaints only when its fill moves by a pixel, or its label changes
class WinBar : public UI
{
public:
    WinBar(int16_t x = 0, int16_t y = 0, int16_t w = 0, int16_t h = 12, Color fill = Theme::BAR,
           Color hi = Theme::BAR_HI)
        : UI(x, y, w, h), fill(fill), hi(hi)
    {
    }

    void setValue(double fraction);
    void setColors(Color newFill, Color newHi);
    // Centered over the bar, in the text colour over the track and white over the fill
    void setLabel(const char *text);
    void draw(TFT_eSPI &tft) override;

private:
    Color fill;
    Color hi;
    int16_t filled = 0;
    char label[24] = "";
};

class WinButton : public UI
{
public:
    WinButton(int16_t x = 0, int16_t y = 0, int16_t w = 0, int16_t h = 22) : UI(x, y, w, h) {}

    void setLabel(const char *text);
    void setFocused(bool on);
    void setEnabled(bool on);
    void draw(TFT_eSPI &tft) override { Win::button(tft, x, y, w, h, label, focused, enabled); }

private:
    char label[28] = "";
    bool focused = false;
    bool enabled = true;
};

// The selection frame around a row, drawn in the face colour when off so moving it leaves
// nothing behind. Registered after the row's texts, which stay inside it.
class FocusFrame : public UI
{
public:
    FocusFrame(int16_t x = 0, int16_t y = 0, int16_t w = 0, int16_t h = 0) : UI(x, y, w, h) {}

    void setOn(bool value)
    {
        if (on != value)
        {
            on = value;
            markDirty();
        }
    }
    void draw(TFT_eSPI &tft) override { Win::focus(tft, x, y, w, h, on); }

private:
    bool on = false;
};

// A raised or sunken panel, a group box's edge
class WinPanel : public UI
{
public:
    WinPanel(int16_t x, int16_t y, int16_t w, int16_t h, bool raised, bool filled = false)
        : UI(x, y, w, h), raised(raised), filled(filled)
    {
    }
    void draw(TFT_eSPI &tft) override;

private:
    bool raised;
    bool filled;
};

// A 16x16 RGB565 icon, black transparent, drawn over the face
class Icon16 : public UI
{
public:
    Icon16(int16_t x = 0, int16_t y = 0) : UI(x, y, 16, 16) {}
    void setPixels(const uint16_t *newPixels)
    {
        if (pixels != newPixels)
        {
            pixels = newPixels;
            markDirty();
        }
    }
    void draw(TFT_eSPI &tft) override;

private:
    const uint16_t *pixels = nullptr;
};

#endif // UI_WIN_H
