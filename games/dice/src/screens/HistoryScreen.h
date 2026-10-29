#ifndef HISTORY_SCREEN_H
#define HISTORY_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "ListView.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../dice/History.h"

// Every result since the board woke, newest first, with what made it and how long ago
class HistoryScreen : public Screen
{
public:
    HistoryScreen(const History &history, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    bool onRepeat(Key key) override;

private:
    static const uint8_t ROWS = 9;

    Header header;
    ListRow rows[ROWS];
    Text emptyText;
    Text hintText;

    const History &history;
    SoundManager &sound;
    ListView list;
    uint32_t shownVersion = 0;

    void refresh(unsigned long now);
};

#endif // HISTORY_SCREEN_H
