#ifndef ACTION_PANEL_H
#define ACTION_PANEL_H

#include "../Screen.h"
#include "../../ui/Text.h"
#include "../../ui/Box.h"
#include "../../ui/Image.h"
#include "../../ui/CooldownRing.h"
#include "../../game/CombatState.h"

// The three physical buttons, stacked like the device (Up spell, Confirm strike, Down
// guard), each in a ring that fills as its cooldown recovers, and the automatic attack in a
// card of its own beside them.
class ActionPanel
{
private:
    struct ActionRow
    {
        CooldownRing ring;
        Text name;
        Text detail; // The hit it deals when ready, the time left when not
    };
    enum Action : uint8_t
    {
        SPELL,
        STRIKE,
        GUARD,
        ACTION_COUNT
    };
    ActionRow actions[ACTION_COUNT];

    Box autoCard;
    Text autoTitle;
    CooldownRing autoRing;
    Image autoIcon;
    Text autoHit;
    Text autoNote;

    CombatState &combat;

    void refreshCooldown(ActionRow &row, unsigned long left, unsigned long total, uint32_t damage, bool fighting);
    void refreshGuard(unsigned long now, bool fighting);
    void refreshSpell(unsigned long now, bool fighting);
    void refreshAutoAttack(unsigned long now, bool fighting);

public:
    explicit ActionPanel(CombatState &combat);

    void addTo(Screen &screen);
    void setVisible(bool visible);
    void refresh(unsigned long now, bool fighting);
};

#endif // ACTION_PANEL_H
