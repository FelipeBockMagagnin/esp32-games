#ifndef GAME_LIST_SCREEN_H
#define GAME_LIST_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "ListView.h"
#include "Text.h"
#include "../core/SoundManager.h"

// Every game built in, title over author, marked when it has a save; Confirm starts one
class GameListScreen : public Screen
{
public:
    static const uint16_t MAX_GAMES = 64;

    GameListScreen(const char *nameSpace, void (*play)(uint16_t index), SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    bool onRepeat(Key key) override;
    void onConfirmPress() override;

    uint16_t getSelected() const { return list.getSelected(); }
    void setSelected(uint16_t index);

private:
    static const uint8_t ROWS = 6;

    Header header;
    ListRow rows[ROWS];
    Text emptyText;
    Text hintText;

    const char *nameSpace;
    void (*play)(uint16_t index);
    SoundManager &sound;
    ListView list;
    bool saved[MAX_GAMES] = {};

    void refresh();
};

#endif // GAME_LIST_SCREEN_H
