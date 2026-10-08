#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/EnemySprite.h"
#include "../ui/Box.h"
#include "../ui/ButtonBadge.h"
#include "../ui/CooldownRing.h"
#include "../ui/DropBanner.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"

// Shows the fight that CombatState is already running, built around three questions a
// player kept getting wrong: which button attacks, when it can be pressed again, and when
// the enemy is about to hit. Kept sparse on purpose: each number here is one the player
// acts on or plays for.
//
// The enemy's next hit charges up in a bar that turns red just before it lands. The three
// physical buttons are drawn where they sit on the device (Up casts the equipped amulet's
// spell, the red Confirm strikes, Down guards), each inside a ring that fills as its
// cooldown recovers and gets a gold outline when it can be pressed. The automatic attack
// has its own ring, so it reads as something that happens by itself. Every hit, both ways,
// shows its damage beside the health bar it took off. An item drop takes over the enemy's
// spot with a big card.
class CombatScreen : public Screen
{
private:
    Header header;

    // Enemy block
    Text enemyName;
    EnemySprite enemySprite;
    DropBanner dropBanner; // Over the sprite while a drop is being announced

    // The enemy's attack and defense in the column left of it, to read against the hits
    // shown on the action buttons. Registered after the banner like the drop list.
    Text enemyAtkLabel;
    Text enemyAtkValue;
    Text enemyDefLabel;
    Text enemyDefValue;

    // What the enemy can drop, beside it: icon and chance, the chance green for an item
    // not owned yet and gold for one that would only level up. Registered after the
    // banner, which covers them while it is up and has them redrawn when it goes.
    static const uint8_t MAX_DROPS = 6;
    Text dropsCaption;
    Image dropIcon[MAX_DROPS];
    Text dropPct[MAX_DROPS];

    ProgressBar enemyHpBar;
    Text enemyHitText; // The player's last hit, beside the bar it came off
    ProgressBar enemyAttackBar; // Fills up towards the enemy's next hit

    // One status line: gold from a kill, knocked out, or a zone out of reach
    Text messageText;

    // One action per physical button, top to bottom like the device: the button inside
    // its cooldown ring, the action's name, and under it the hit it deals when ready or
    // the time left when not
    struct ActionRow
    {
        CooldownRing ring;
        Text name;
        Text detail;
    };
    static const uint8_t ACTION_COUNT = 3;
    enum Action : uint8_t
    {
        SPELL,
        STRIKE,
        GUARD
    };
    ActionRow actions[ACTION_COUNT];

    // The automatic attack, in a card of its own since no button fires it
    Box autoCard;
    Text autoTitle;
    CooldownRing autoRing;
    Image autoIcon;
    Text autoHit;
    Text autoNote;

    // Player block: attack and defense to read against the enemy's card, then health
    Text atkLabel;
    Text atkValue;
    Text defLabel;
    Text defValue;
    ProgressBar playerHpBar;
    Text playerHitText; // The enemy's last hit, beside the bar it came off

    CombatState &combat;
    SoundManager &sound;

    unsigned long lastRefresh;
    uint32_t seenKills;
    uint32_t seenDrops;
    uint32_t seenHits;
    uint32_t seenHurts;

    unsigned long messageUntil;
    unsigned long dropUntil;
    unsigned long enemyHitUntil;
    unsigned long playerHitUntil;
    unsigned long playerFlashUntil;

    void showFightElements(bool visible);
    void refreshDrops(const EnemyDef &enemy);
    void redrawSideColumns();
    void showMessage(unsigned long now, const char *text, uint16_t color, unsigned long durationMs);
    void refreshEnemy(unsigned long now);
    void handleEvents(unsigned long now);
    void refreshEnemyAttack(unsigned long now, bool fighting);
    void refreshActions(unsigned long now, bool fighting);
    void refreshSpell(unsigned long now, bool fighting);
    void refreshCooldown(ActionRow &row, unsigned long left, unsigned long total, uint32_t damage, bool fighting);
    void refreshAutoAttack(unsigned long now, bool fighting);
    void refreshPlayer(unsigned long now);

public:
    CombatScreen(CombatState &combat, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
};

#endif // COMBAT_SCREEN_H
