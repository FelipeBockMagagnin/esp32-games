#ifndef LIBRARY_SCREEN_H
#define LIBRARY_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "../app/Transport.h"
#include "../core/ScreenManager.h"

// The shelf: the records and the settings, then a record's tracks. OK opens or plays, Left goes
// back up, and from the records back to the player.
class LibraryScreen : public Screen
{
public:
    LibraryScreen(Transport &transport, ScreenManager &screenManager, Screen &settings);

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    bool onRepeat(Key key) override;

private:
    static const uint8_t ROWS = 8;

    Header header;
    ListRow rows[ROWS];
    Transport &transport;
    ScreenManager &screenManager;
    Screen &settings;
    int16_t album = -1; // -1 the shelf of records
    uint8_t selected = 0;
    uint8_t first = 0;

    uint8_t count() const;
    void move(int8_t delta);
    void refresh();
};

#endif // LIBRARY_SCREEN_H
