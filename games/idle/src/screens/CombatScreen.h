#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ProgressBar.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"
#include "combat/EnemyPanel.h"
#include "combat/ActionPanel.h"

// Shows the fight that CombatState is already running. Kept sparse on purpose: each
// number here is one the player acts on or plays for.
class CombatScreen : public Screen
{
private:
    Header header;

    EnemyPanel enemyPanel;

    ProgressBar enemyHpBar;
    Text enemyHitText;
    ProgressBar enemyAttackBar; // Charges up towards the enemy's next hit

    Text messageText; // Kill gold, knocked out, or a zone out of reach

    ActionPanel actionPanel;

    Text atkLabel;
    Text atkValue;
    Text defLabel;
    Text defValue;
    Text regenLabel;
    Text regenValue;
    ProgressBar playerHpBar;
    Text playerHitText;

    CombatState &combat;
    SoundManager &sound;

    uint32_t seenKills;
    uint32_t seenDrops;
    uint32_t seenHits;
    uint32_t seenHurts;
    uint32_t seenSecondWinds;

    unsigned long messageUntil;
    unsigned long enemyHitUntil;
    unsigned long playerHitUntil;
    unsigned long playerFlashUntil;

    void showFightElements(bool visible);
    void showMessage(unsigned long now, const char *text, uint16_t color, unsigned long durationMs);
    void handleEvents(unsigned long now);
    void refreshEnemyAttack(unsigned long now, bool fighting);
    void refreshPlayer(unsigned long now);
    void refreshStatus(unsigned long now, bool fighting);

public:
    CombatScreen(CombatState &combat, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
};

#endif // COMBAT_SCREEN_H
