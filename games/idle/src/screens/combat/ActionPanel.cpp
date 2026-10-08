#include "ActionPanel.h"
#include "CombatStyle.h"
#include "../../ui/ButtonBadge.h"
#include "../../assets/Assets.h"

static const uint16_t AUTO_COLOR = Theme::ATTACK;

static const int16_t RING_CX = 18;
static const int16_t RING_R = 11;
static const int16_t ACTION_NAME_X = 35;
static const int16_t SPELL_CY = 237;
static const int16_t STRIKE_CY = 260;
static const int16_t GUARD_CY = 283;
static const int16_t NAME_DY = -8;
static const int16_t DETAIL_DY = 1;

static const int16_t AUTO_X = 128;
static const int16_t AUTO_Y = 225;
static const int16_t AUTO_W = 106;
static const int16_t AUTO_H = 70;
static const int16_t AUTO_CX = AUTO_X + AUTO_W / 2;
static const int16_t AUTO_RING_CY = AUTO_Y + 30;
static const int16_t AUTO_RING_R = 14;

#define ACTION_ROW(cy, kind, label)                                               \
    {                                                                             \
        CooldownRing(RING_CX, cy, RING_R, kind),                                  \
            Text(ACTION_NAME_X, (cy) + NAME_DY, label, Theme::TEXT_2, 1),         \
            Text(ACTION_NAME_X, (cy) + DETAIL_DY, "", Theme::TEXT_DIM, 1)         \
    }

ActionPanel::ActionPanel(CombatState &combat)
    : actions{ACTION_ROW(SPELL_CY, ButtonBadge::UP, "SPELL"),
              ACTION_ROW(STRIKE_CY, ButtonBadge::CONFIRM, "STRIKE"),
              ACTION_ROW(GUARD_CY, ButtonBadge::DOWN, "GUARD")},
      autoCard(AUTO_X, AUTO_Y, AUTO_W, AUTO_H, Theme::FRAME, true, Theme::CARD, true, 4),
      autoTitle(AUTO_CX, AUTO_Y + 5, "AUTO", Theme::TEXT_2, 1, TC_DATUM),
      autoRing(AUTO_CX, AUTO_RING_CY, AUTO_RING_R),
      autoIcon(AUTO_CX - 8, AUTO_RING_CY - 8, 16, 16, image_sword_02b_pixels, true, 0x0000),
      autoHit(AUTO_CX, AUTO_Y + 50, "", Theme::TEXT, 1, TC_DATUM),
      autoNote(AUTO_CX, AUTO_Y + 60, "", Theme::TEXT_2, 1, TC_DATUM),
      combat(combat)
{
}

#undef ACTION_ROW

void ActionPanel::addTo(Screen &screen)
{
    for (uint8_t i = 0; i < ACTION_COUNT; i++)
    {
        screen.addElement(&actions[i].ring);
        screen.addElement(&actions[i].name);
        screen.addElement(&actions[i].detail);
    }

    // The icon after the ring, so it lands inside it
    screen.addElement(&autoCard);
    screen.addElement(&autoTitle, Theme::CARD);
    screen.addElement(&autoRing, Theme::CARD);
    screen.addElement(&autoIcon, Theme::CARD);
    screen.addElement(&autoHit, Theme::CARD);
    screen.addElement(&autoNote, Theme::CARD);
}

// The auto ring and icon decide their own visibility in refresh(), so they are only ever
// hidden here; toggling them twice per refresh would flicker
void ActionPanel::setVisible(bool visible)
{
    for (uint8_t i = 0; i < ACTION_COUNT; i++)
    {
        actions[i].ring.setVisible(visible);
        actions[i].name.setVisible(visible);
        actions[i].detail.setVisible(visible);
    }
    autoCard.setVisible(visible);
    autoTitle.setVisible(visible);
    autoHit.setVisible(visible);
    autoNote.setVisible(visible);
    if (!visible)
    {
        autoRing.setVisible(false);
        autoIcon.setVisible(false);
    }
}

void ActionPanel::refresh(unsigned long now, bool fighting)
{
    refreshCooldown(actions[STRIKE], combat.getStrikeCooldownRemaining(now), STRIKE_COOLDOWN_MS,
                    combat.getStrikeDamage(), fighting);
    refreshSpell(now, fighting);
    refreshGuard(now, fighting);
    refreshAutoAttack(now, fighting);
}

// A hit that cannot beat the enemy's defense says so instead of inviting a useless press
void ActionPanel::refreshCooldown(ActionRow &row, unsigned long left, unsigned long total, uint32_t damage,
                                  bool fighting)
{
    char buf[20];
    bool blocked = fighting && damage == 0;
    bool ready = fighting && !blocked && left == 0;

    // Cooldowns keep running between enemies, so the ring shows them even then
    row.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / total));
    row.ring.setRingColor(ready ? CombatStyle::READY : Theme::TEXT_DIM);
    row.ring.setOutline(ready);
    row.ring.setCapDimmed(!fighting || blocked);
    row.name.setColor(ready ? Theme::TEXT : Theme::TEXT_2);

    if (blocked)
    {
        row.detail.setText("blocked");
        row.detail.setColor(CombatStyle::BLOCKED);
    }
    else if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(Theme::TEXT_DIM);
    }
    else
    {
        snprintf(buf, sizeof(buf), "hits %u", (unsigned)damage);
        row.detail.setText(fighting ? buf : "");
        row.detail.setColor(CombatStyle::READY);
    }
}

void ActionPanel::refreshGuard(unsigned long now, bool fighting)
{
    ActionRow &guard = actions[GUARD];
    char buf[20];
    unsigned long left = combat.getGuardCooldownRemaining(now);
    bool guarding = combat.isGuarding(now);
    bool ready = fighting && left == 0;
    guard.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / GUARD_COOLDOWN_MS));
    guard.ring.setRingColor(guarding ? CombatStyle::ACTIVE : ready ? CombatStyle::READY : Theme::TEXT_DIM);
    guard.ring.setOutline(ready);
    guard.ring.setCapDimmed(!fighting);
    guard.name.setColor(ready || guarding ? Theme::TEXT : Theme::TEXT_2);
    if (guarding)
    {
        unsigned long up = combat.getGuardRemaining(now);
        snprintf(buf, sizeof(buf), "on %lu.%lus", up / 1000, (up % 1000) / 100);
        guard.detail.setText(buf);
        guard.detail.setColor(CombatStyle::ACTIVE);
    }
    else if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        guard.detail.setText(buf);
        guard.detail.setColor(Theme::TEXT_DIM);
    }
    else
    {
        snprintf(buf, sizeof(buf), "-%u%% dmg", (unsigned)(100 - GUARD_DAMAGE_PERCENT));
        guard.detail.setText(fighting ? buf : "");
        guard.detail.setColor(CombatStyle::READY);
    }
}

// The row takes the equipped amulet's spell name. Without an amulet it stays, dimmed, so
// the reward is discoverable.
void ActionPanel::refreshSpell(unsigned long now, bool fighting)
{
    ActionRow &row = actions[SPELL];
    const SpellDef *spell = combat.getSpell();
    char buf[20];

    if (spell == nullptr)
    {
        row.name.setText("SPELL");
        row.name.setColor(Theme::TEXT_DIM);
        row.ring.setProgress(0);
        row.ring.setRingColor(Theme::TEXT_DIM);
        row.ring.setOutline(false);
        row.ring.setCapDimmed(true);
        row.detail.setText("equip amulet");
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }

    unsigned long left = combat.getSpellCooldownRemaining(now);
    bool warded = combat.isWarded(now);
    bool useless = combat.isSpellUseless();
    bool ready = fighting && left == 0 && !useless;

    row.name.setText(spell->name);
    row.name.setColor(ready || warded ? Theme::TEXT : Theme::TEXT_2);
    row.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / spell->cooldownMs));
    row.ring.setRingColor(warded ? CombatStyle::ACTIVE : ready ? CombatStyle::READY : Theme::TEXT_DIM);
    row.ring.setOutline(ready);
    row.ring.setCapDimmed(!fighting);

    if (warded)
    {
        unsigned long up = combat.getWardRemaining(now);
        snprintf(buf, sizeof(buf), "on %lu.%lus", up / 1000, (up % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(CombatStyle::ACTIVE);
        return;
    }
    if (combat.isMidasArmed())
    {
        snprintf(buf, sizeof(buf), "next kill x%u", (unsigned)spell->power);
        row.detail.setText(buf);
        row.detail.setColor(Theme::GOLD);
        return;
    }
    if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }
    if (useless)
    {
        row.detail.setText("full HP");
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }

    switch (spell->effect)
    {
    case SpellEffect::HEAL:
        snprintf(buf, sizeof(buf), "heal %u",
                 (unsigned)(combat.getPlayerStats().maxHp * spell->power / 100));
        break;
    case SpellEffect::STUN:
        snprintf(buf, sizeof(buf), "skip a hit");
        break;
    case SpellEffect::MIDAS:
        snprintf(buf, sizeof(buf), "x%u gold", (unsigned)spell->power);
        break;
    case SpellEffect::WARD:
        snprintf(buf, sizeof(buf), "block %us", (unsigned)(spell->power / 1000));
        break;
    }
    row.detail.setText(fighting ? buf : "");
    row.detail.setColor(CombatStyle::READY);
}

// When the enemy's defense blocks the auto-attack, the ring goes away and the card points
// at the strike instead
void ActionPanel::refreshAutoAttack(unsigned long now, bool fighting)
{
    char buf[20];
    uint32_t damage = combat.getAutoAttackDamage();
    bool working = damage > 0;

    autoRing.setVisible(working);
    autoIcon.setVisible(working);
    if (working)
    {
        unsigned long left = combat.getAutoAttackRemaining(now);
        autoRing.setProgress(fighting ? (uint16_t)(1000 - left * 1000 / AUTO_ATTACK_MS) : 0);
        autoRing.setRingColor(AUTO_COLOR);
        snprintf(buf, sizeof(buf), "hits %u", (unsigned)damage);
        autoHit.setText(buf);
        autoHit.setColor(Theme::TEXT);
        autoNote.setText("");
    }
    else
    {
        autoHit.setText("blocked");
        autoHit.setColor(CombatStyle::BLOCKED);
        autoNote.setText("use STRIKE");
    }
}
