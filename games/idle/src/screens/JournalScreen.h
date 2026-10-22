#ifndef JOURNAL_SCREEN_H
#define JOURNAL_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/IconStrip.h"
#include "../ui/ButtonBadge.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../game/CombatState.h"
#include "../game/Achievements.h"
#include "../game/Ascension.h"
#include "../game/Journal.h"
#include "../managers/SoundManager.h"

// Two pages, Confirm switching between them: the player's records, this run's and every
// run's, and a bestiary of every enemy with what it drops and how the current gear fares.
// The pages share the body, so a switch clears it with a box registered before both.
class JournalScreen : public Screen
{
public:
    JournalScreen(const GameState &game, const Inventory &inventory, const CombatState &combat,
                  const Achievements &achievements, const Ascension &ascension, const Journal &journal,
                  SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;

private:
    static const uint8_t LINES = 21;
    static const uint8_t MAX_ENTRIES = 32;
    static const uint8_t BEAST_ROWS = 7;

    struct Entry
    {
        const char *label;
        char value[24];
        bool heading;
    };

    Header header;
    Box bodyClear;
    ButtonBadge pageBadge;
    Text statsTab;
    Text bestiaryTab;
    Text scrollHint;

    Text lineLabel[LINES];
    Text lineValue[LINES];

    ListRow rows[BEAST_ROWS];
    ListView beasts;
    Box beastPanel;
    Text beastStats;
    Text beastFight;
    IconStrip beastDrops;
    Text beastReward;

    const GameState &game;
    const Inventory &inventory;
    const CombatState &combat;
    const Achievements &achievements;
    const Ascension &ascension;
    const Journal &journal;
    SoundManager &sound;

    bool bestiary;
    uint8_t scroll;
    Entry entries[MAX_ENTRIES];
    uint8_t entryCount;

    void showPage(bool visible, bool bestiaryPage);
    void buildEntries();
    Entry &add(const char *label, bool heading = false);
    void refreshStats();
    void refreshBestiary();
    bool findEnemy(uint16_t index, uint8_t &zone, uint8_t &enemy) const;
    bool isMet(uint8_t zone) const;
};

#endif // JOURNAL_SCREEN_H
