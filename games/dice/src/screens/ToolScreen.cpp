#include "ToolScreen.h"

static const int16_t ROW_X = 5;
static const int16_t ACTION_Y = 296;
static const int16_t FIELD_PITCH = FieldRow::HEIGHT + 1;

ToolScreen::ToolScreen(const char *title, uint8_t fieldCount, SoundManager &sound, void (*saved)())
    : header(title),
      action(ROW_X, ACTION_Y),
      sound(sound),
      fieldCount(fieldCount < MAX_FIELDS ? fieldCount : MAX_FIELDS),
      selected(this->fieldCount),
      saved(saved)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < this->fieldCount; i++)
    {
        fields[i].setPosition(ROW_X, ACTION_Y - 3 - (this->fieldCount - i) * FIELD_PITCH);
        addElement(&fields[i]);
    }
    addElement(&action);
}

void ToolScreen::refreshRows()
{
    refreshFields();
    for (uint8_t i = 0; i < fieldCount; i++)
    {
        fields[i].setState(i != selected ? FieldRow::NORMAL : (editing ? FieldRow::EDITING : FieldRow::SELECTED));
    }
    action.setSelected(onAction());
    forceRefresh();
}

const char *ToolScreen::hint() const
{
    if (editing)
    {
        return "Up/Down change - Confirm done";
    }
    return onAction() ? nullptr : "Confirm to change";
}

void ToolScreen::onEnter(TFT_eSPI &tft)
{
    selected = fieldCount;
    refreshRows();
    Screen::onEnter(tft);
}

void ToolScreen::onExit()
{
    finishEdit();
}

void ToolScreen::finishEdit()
{
    if (!editing)
    {
        return;
    }
    editing = false;
    if (changed && saved != nullptr)
    {
        saved();
    }
    changed = false;
    refreshRows();
}

void ToolScreen::move(int8_t delta)
{
    selected = (selected + fieldCount + 1 + delta) % (fieldCount + 1);
    refreshRows();
    sound.playMenu();
}

void ToolScreen::edit(int16_t delta)
{
    if (!changeField(selected, delta))
    {
        sound.playError();
        return;
    }
    changed = true;
    refreshRows();
    sound.playMenu();
}

void ToolScreen::onUpPress()
{
    if (isBusy())
    {
        return;
    }
    if (editing)
    {
        edit(1);
    }
    else
    {
        move(-1);
    }
}

void ToolScreen::onDownPress()
{
    if (isBusy())
    {
        return;
    }
    if (editing)
    {
        edit(-1);
    }
    else
    {
        move(1);
    }
}

void ToolScreen::onConfirmPress()
{
    if (isBusy())
    {
        return;
    }
    if (onAction())
    {
        act();
        return;
    }
    if (editing)
    {
        finishEdit();
    }
    else
    {
        editing = true;
        refreshRows();
    }
    sound.playMenu();
}

bool ToolScreen::onRepeat(Key key)
{
    if (!editing)
    {
        return false;
    }
    switch (key)
    {
    case Key::UP:
        edit(1);
        return true;
    case Key::DOWN:
        edit(-1);
        return true;
    case Key::LEFT:
        edit(-bigStep(selected, -1));
        return true;
    case Key::RIGHT:
        edit(bigStep(selected, 1));
        return true;
    default:
        return false;
    }
}

bool ToolScreen::onBackPress()
{
    if (!editing)
    {
        return false;
    }
    edit(-bigStep(selected, -1));
    return true;
}

bool ToolScreen::onRightPress()
{
    if (!editing)
    {
        return false;
    }
    edit(bigStep(selected, 1));
    return true;
}
