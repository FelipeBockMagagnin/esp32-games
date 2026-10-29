#ifndef TOOL_SCREEN_H
#define TOOL_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "../core/SoundManager.h"
#include "../ui/FieldRow.h"

// What Roll, Draw and Wheel share: their settings as rows above the row that acts, at the
// bottom of the screen. Up and Down move between the rows and Confirm acts; Confirm on a
// setting edits it instead, until the next Confirm: Up and Down then change it by one, Left
// and Right by a bigger step, and holding any of them repeats. Left and Right step through
// the screens otherwise. Every visit starts on the row that acts.
class ToolScreen : public Screen
{
public:
    void onEnter(TFT_eSPI &tft) override;
    void onExit() override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;
    bool onBackPress() override;
    bool onRightPress() override;

protected:
    static const uint8_t MAX_FIELDS = 4;

    // `saved` runs when an edit that changed something ends, to store the settings
    ToolScreen(const char *title, uint8_t fieldCount, SoundManager &sound, void (*saved)());

    Header header;
    FieldRow fields[MAX_FIELDS];
    ActionRow action;
    SoundManager &sound;

    // Fills the rows' values from the settings
    virtual void refreshFields() = 0;
    // False when the field is already at its limit that way
    virtual bool changeField(uint8_t field, int16_t delta) = 0;
    // The step Left (direction -1) or Right (1) takes on this field
    virtual int16_t bigStep(uint8_t field, int8_t direction) const { return 1; }
    virtual void act() = 0;
    // While true (dice tumbling, the wheel turning) every press but a screen change waits
    virtual bool isBusy() const { return false; }

    bool isEditing() const { return editing; }
    bool onAction() const { return selected == fieldCount; }
    // What the screen's caption should say for the row the cursor is on, or nullptr on the
    // action row, where the screen shows its own result
    const char *hint() const;
    void refreshRows();

private:
    uint8_t fieldCount;
    uint8_t selected;
    bool editing = false;
    bool changed = false;
    void (*saved)();

    void move(int8_t delta);
    void edit(int16_t delta);
    void finishEdit();
};

#endif // TOOL_SCREEN_H
