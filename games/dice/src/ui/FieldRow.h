#ifndef UI_FIELD_ROW_H
#define UI_FIELD_ROW_H

#include "UI.h"

// One setting of a tool on a compact row: the label on the left, the value on the right.
// While it is edited, arrows beside the value point at the buttons that change it.
class FieldRow : public UI
{
public:
    enum State : uint8_t
    {
        NORMAL,
        SELECTED,
        EDITING
    };

    static const int16_t HEIGHT = 22;

    FieldRow(int16_t x = 0, int16_t y = 0, int16_t w = 230) : UI(x, y, w, HEIGHT) {}

    void setLabel(const char *v) { assign(label, v); }
    void setValue(const char *v) { assign(value, v); }
    void setState(State v)
    {
        if (state != v)
        {
            state = v;
            markDirty();
        }
    }
    // A value that does not apply right now, like a keep with a single die
    void setMuted(bool v)
    {
        if (muted != v)
        {
            muted = v;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    String label;
    String value;
    State state = NORMAL;
    bool muted = false;

    void assign(String &field, const char *v)
    {
        if (field != v)
        {
            field = v;
            markDirty();
        }
    }
};

// The row that does the tool's job, with the Confirm button's red cap beside its label
class ActionRow : public UI
{
public:
    static const int16_t HEIGHT = 22;

    ActionRow(int16_t x = 0, int16_t y = 0, int16_t w = 230) : UI(x, y, w, HEIGHT) {}

    void setLabel(const char *v)
    {
        if (label != v)
        {
            label = v;
            markDirty();
        }
    }
    void setSelected(bool v)
    {
        if (selected != v)
        {
            selected = v;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    String label;
    bool selected = false;
};

#endif // UI_FIELD_ROW_H
