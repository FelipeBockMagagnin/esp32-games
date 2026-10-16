#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ProgressBar.h"
#include "../ui/Ellipse.h"
#include "../game/GameState.h"
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
    // Kill gold lands here, so the balance it adds to is in sight
    Ellipse goldCoin;
    Text goldText;
    ProgressBar playerHpBar;
    Text playerHitText;

    CombatState &combat;
    const GameState &game;
    SoundManager &sound;

    uint32_t seenKills;
    uint32_t seenDrops;
    uint32_t seenHits;
    uint32_t seenHurts;
    uint32_t seenSecondWinds;
    uint32_t seenWeakPoints;
    uint32_t seenFuries;

    unsigned long messageUntil;
    unsigned long enemyHitUntil;
    unsigned long playerHitUntil;
    unsigned long playerFlashUntil;
    unsigned long playerHealUntil;
    unsigned long enemyFlashUntil;

    void showFightElements(bool visible);
    void showMessage(unsigned long now, const char *text, Color color, unsigned long durationMs);
    void handleEvents(unsigned long now);
    void refreshEnemyAttack(unsigned long now, bool fighting);
    void refreshPlayer(unsigned long now);
    void refreshGold();
    void refreshStatus(unsigned long now, bool fighting);
    Color hint(unsigned long now, char *buf, size_t size);

public:
    CombatScreen(CombatState &combat, const GameState &game, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
};

#endif // COMBAT_SCREEN_H
