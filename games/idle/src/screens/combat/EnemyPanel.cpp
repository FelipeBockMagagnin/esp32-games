#include "EnemyPanel.h"
#include "../../game/Inventory.h"

static const int16_t CENTER_X = 120;
static const int16_t NAME_Y = 31;

static const int16_t SPRITE_X = (240 - EnemySprite::SIZE) / 2;
static const int16_t SPRITE_Y = 50;
static const int16_t BANNER_X = 6;
static const int16_t BANNER_W = 228;

static const int16_t STATS_CENTER_X = SPRITE_X / 2 + 1;
static const int16_t ATK_LABEL_Y = SPRITE_Y + 2;
static const int16_t ATK_VALUE_Y = ATK_LABEL_Y + 11;
static const int16_t DEF_LABEL_Y = ATK_LABEL_Y + 36;
static const int16_t DEF_VALUE_Y = DEF_LABEL_Y + 11;
static const int16_t BOSS_Y = DEF_LABEL_Y + 40;

static const int16_t DROPS_X = SPRITE_X + EnemySprite::SIZE + 4;
static const int16_t DROPS_CENTER_X = (DROPS_X + 234) / 2;
static const int16_t DROPS_Y = SPRITE_Y + 2;
static const int16_t DROP_ROW_Y = DROPS_Y + 12;
static const int16_t DROP_PITCH = 19;
static const int16_t DROP_ICON_SIZE = 16;
static const int16_t DROP_PCT_DX = DROP_ICON_SIZE + 3;

EnemyPanel::EnemyPanel(CombatState &combat)
    : name(CENTER_X, NAME_Y, "", Theme::ENEMY_NAME, 2, TC_DATUM),
      sprite(SPRITE_X, SPRITE_Y),
      banner(BANNER_X, SPRITE_Y, BANNER_W, EnemySprite::SIZE,
             SPRITE_X, SPRITE_Y, EnemySprite::SIZE, EnemySprite::SIZE),
      atkLabel(STATS_CENTER_X, ATK_LABEL_Y, "ATK", Theme::TEXT_DIM, 1, TC_DATUM),
      atkValue(STATS_CENTER_X, ATK_VALUE_Y, "", Theme::ATTACK, 2, TC_DATUM),
      defLabel(STATS_CENTER_X, DEF_LABEL_Y, "DEF", Theme::TEXT_DIM, 1, TC_DATUM),
      defValue(STATS_CENTER_X, DEF_VALUE_Y, "", Theme::DEFENSE, 2, TC_DATUM),
      bossTag(STATS_CENTER_X, BOSS_Y, "BOSS", Theme::BAD, 2, TC_DATUM),
      dropsCaption(DROPS_CENTER_X, DROPS_Y, "DROPS", Theme::TEXT_DIM, 1, TC_DATUM),
      combat(combat),
      bannerUntil(0),
      wasRespawning(false)
{
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        int16_t rowY = DROP_ROW_Y + i * DROP_PITCH;
        dropIcon[i].setPosition(DROPS_X, rowY);
        dropIcon[i].setVisible(false);
        // 8px text centred beside a 16px icon
        dropPct[i].setPosition(DROPS_X + DROP_PCT_DX, rowY + 4);
        dropPct[i].setVisible(false);
    }
}

void EnemyPanel::addTo(Screen &screen)
{
    screen.addElement(&name);
    screen.addElement(&sprite);
    screen.addElement(&banner);
    screen.addElement(&atkLabel);
    screen.addElement(&atkValue);
    screen.addElement(&defLabel);
    screen.addElement(&defValue);
    screen.addElement(&bossTag);
    screen.addElement(&dropsCaption);
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        screen.addElement(&dropIcon[i]);
        screen.addElement(&dropPct[i]);
    }
}

// The drop rows decide their own visibility in refresh(), so they are only ever hidden here
void EnemyPanel::setVisible(bool visible)
{
    name.setVisible(visible);
    sprite.setVisible(visible);
    atkLabel.setVisible(visible);
    atkValue.setVisible(visible);
    defLabel.setVisible(visible);
    defValue.setVisible(visible);
    dropsCaption.setVisible(visible);
    if (!visible)
    {
        bossTag.setVisible(false);
        banner.setVisible(false);
        for (uint8_t i = 0; i < MAX_DROPS; i++)
        {
            dropIcon[i].setVisible(false);
            dropPct[i].setVisible(false);
        }
    }
}

void EnemyPanel::hideBanner()
{
    banner.setVisible(false);
}

void EnemyPanel::onHit(uint32_t damage, bool heavy)
{
    sprite.hit(heavy);
    sprite.addDamage(damage, heavy);
}

void EnemyPanel::onAttack()
{
    sprite.lunge();
}

void EnemyPanel::onCast(uint16_t color, bool inward)
{
    sprite.cast(color, inward);
}

void EnemyPanel::onKill()
{
    sprite.kill();
}

void EnemyPanel::stopAnimations()
{
    sprite.stopAnimations();
    wasRespawning = false;
}

// The banner covers the sprite, and a frame drawn under it would paint over it
void EnemyPanel::animate(unsigned long now)
{
    if (!banner.isVisible())
    {
        sprite.update(now);
    }
}

void EnemyPanel::showDrop(uint8_t itemId, bool isNew, uint8_t level, uint8_t others, unsigned long until)
{
    banner.show(itemId, isNew, level, others);
    bannerUntil = until;
}

void EnemyPanel::refreshSideColumns(const EnemyDef &enemy)
{
    const Inventory &inventory = combat.getInventory();
    uint8_t count = enemy.dropCount < MAX_DROPS ? enemy.dropCount : MAX_DROPS;
    char buf[8];

    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.attack);
    atkValue.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.defense);
    defValue.setText(buf);
    bossTag.setVisible(enemy.boss);

    uint8_t slot = 0;
    for (uint8_t i = 0; i < enemy.dropCount && slot < MAX_DROPS; i++)
    {
        const DropDef &drop = enemy.drops[i];
        if (!combat.canDrop(drop.itemId))
        {
            continue;
        }

        dropIcon[slot].setVisible(true);
        dropPct[slot].setVisible(true);
        dropIcon[slot].setPixels(ITEMS[drop.itemId].iconPixels, DROP_ICON_SIZE, DROP_ICON_SIZE, true, 0x0000);

        // Basis points: whole percents print plain, rare drops keep their tenth (5.5%)
        uint16_t chance = combat.getDropChanceBp(drop);
        unsigned whole = chance / 100;
        unsigned tenth = (chance % 100) / 10;
        if (tenth == 0)
        {
            snprintf(buf, sizeof(buf), "%u%%", whole);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%u.%u%%", whole, tenth);
        }
        dropPct[slot].setText(buf);
        dropPct[slot].setColor(inventory.isOwned(drop.itemId) ? Theme::GOLD : Theme::GOOD);
        slot++;
    }

    for (uint8_t i = slot; i < MAX_DROPS; i++)
    {
        dropIcon[i].setVisible(false);
        dropPct[i].setVisible(false);
    }
}

void EnemyPanel::redrawSideColumns()
{
    atkLabel.resetDrawn();
    atkValue.resetDrawn();
    defLabel.resetDrawn();
    defValue.resetDrawn();
    bossTag.resetDrawn();
    dropsCaption.resetDrawn();
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        dropIcon[i].resetDrawn();
        dropPct[i].resetDrawn();
    }
}

// While the banner is up nothing under it changes. When it goes, it has only cleared its
// own area outside the sprite, so the sprite and both side columns draw again whole.
void EnemyPanel::refresh(unsigned long now)
{
    const EnemyDef &enemy = combat.getEnemy();
    name.setText(enemy.name);
    name.setColor(enemy.boss ? Theme::BAD : Theme::ENEMY_NAME);

    bool bannerLeaving = false;
    if (banner.isVisible())
    {
        if ((long)(now - bannerUntil) < 0)
        {
            return;
        }
        banner.setVisible(false);
        bannerLeaving = true;
    }

    // Whole seconds, rounded up, so the card repaints the sprite once a second and never shows 0
    bool respawning = combat.getPhase() == CombatState::RESPAWNING;
    unsigned long left = combat.getRespawnRemaining(now);
    sprite.setSprite(respawning ? nullptr : enemy.sprite);
    sprite.setCountdown(respawning ? (uint8_t)(left > 0 ? (left + 999) / 1000 : 1) : 0);
    sprite.setBackground(combat.getZone().background);
    if (wasRespawning && !respawning)
    {
        sprite.spawn();
    }
    wasRespawning = respawning;
    refreshSideColumns(enemy);
    if (bannerLeaving)
    {
        sprite.resetDrawn();
        redrawSideColumns();
    }
}
