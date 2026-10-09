#ifndef ENEMY_PANEL_H
#define ENEMY_PANEL_H

#include "../Screen.h"
#include "../../ui/Text.h"
#include "../../ui/Image.h"
#include "../../ui/EnemySprite.h"
#include "../../ui/DropBanner.h"
#include "../../game/CombatState.h"

// The enemy's name and sprite, its ATK/DEF in the column on the left and its drops on the
// right. A drop banner takes over the sprite's spot; the side columns are registered after
// it so they can be redrawn whole over the area it clears when it goes.
class EnemyPanel
{
private:
    static const uint8_t MAX_DROPS = 6;

    Text name;
    EnemySprite sprite;
    DropBanner banner;

    Text atkLabel;
    Text atkValue;
    Text defLabel;
    Text defValue;
    Text bossTag;

    Text dropsCaption;
    Image dropIcon[MAX_DROPS];
    Text dropPct[MAX_DROPS];

    CombatState &combat;
    unsigned long bannerUntil;

    void refreshSideColumns(const EnemyDef &enemy);
    void redrawSideColumns();

public:
    explicit EnemyPanel(CombatState &combat);

    void addTo(Screen &screen);
    void setVisible(bool visible);
    void hideBanner();
    void onHit(uint32_t damage, bool heavy);
    void onKill();
    void onAttack();
    void stopAnimations();
    void animate(unsigned long now);
    void showDrop(uint8_t itemId, bool isNew, uint8_t level, unsigned long until);
    void refresh(unsigned long now);
};

#endif // ENEMY_PANEL_H
