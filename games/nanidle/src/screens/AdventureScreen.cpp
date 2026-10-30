#include "AdventureScreen.h"
#include "../nan/Format.h"
#include "../ui/Art.h"
#include "../ui/Labels.h"

static const int16_t SPRITE_X = (DISPLAY_W - EnemySprite::SIZE) / 2;
static const int16_t SPRITE_Y = 40;

AdventureScreen::AdventureScreen(Game &game, SoundManager &sound)
    : Screen("Aventura"),
      game(game),
      sound(sound),
      zoneName(label(DISPLAY_W / 2, 25, TC_DATUM)),
      spriteFrame(SPRITE_X - 2, SPRITE_Y - 2, EnemySprite::SIZE + 4, EnemySprite::SIZE + 4, false),
      sprite(SPRITE_X, SPRITE_Y),
      enemyName(label(DISPLAY_W / 2, 174, TC_DATUM)),
      enemyBar(10, 185, 220, 13, Theme::HP, Theme::HP_HI),
      youLabel(label(10, 205)),
      playerBar(48, 202, 182, 13, Theme::BAR, Theme::BAR_HI),
      strikeLabel(label(10, 222)),
      strikeBar(48, 219, 182, 13, Theme::GOLD, Theme::BAR_HI),
      rates(label(DISPLAY_W / 2, 238, TC_DATUM, Theme::TEXT_2)),
      dropLine(label(DISPLAY_W / 2, 254, TC_DATUM, Theme::GOOD)),
      tally(label(DISPLAY_W / 2, 270, TC_DATUM, Theme::TEXT_2))
{
    youLabel.setText("Voce");
    strikeLabel.setText("Golpe");
    addElement(&zoneName);
    addElement(&spriteFrame);
    addElement(&sprite);
    addElement(&enemyName);
    addElement(&enemyBar);
    addElement(&youLabel);
    addElement(&playerBar);
    addElement(&strikeLabel);
    addElement(&strikeBar);
    addElement(&rates);
    addElement(&dropLine);
    addElement(&tally);
    addTaskbar();
}

void AdventureScreen::onEnter(TFT_eSPI &tft)
{
    seenSwings = game.playerSwings();
    seenEnemySwings = game.enemySwings();
    seenKills = game.advKills();
    seenDrops = game.drops();
    seenDeaths = game.advDeaths();
    shownEnemy = nullptr;
    shownZone = 0xFF;
    sprite.stopAnimations();
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void AdventureScreen::update(unsigned long now)
{
    sprite.update(now);
    char buf[24];
    if (game.playerSwings() != seenSwings)
    {
        seenSwings = game.playerSwings();
        formatCompact(game.lastSwing(), buf, sizeof(buf));
        sprite.addDamage(buf, game.lastSwingStrike());
        sprite.hit(game.lastSwingStrike());
        sound.playHit(game.lastSwingStrike());
    }
    if (game.enemySwings() != seenEnemySwings)
    {
        seenEnemySwings = game.enemySwings();
        sprite.lunge();
    }
    if (game.advKills() != seenKills)
    {
        seenKills = game.advKills();
        sprite.kill();
        sound.playKill();
    }
    if (game.drops() != seenDrops)
    {
        seenDrops = game.drops();
        int8_t item = game.lastDrop();
        snprintf(buf, sizeof(buf), "Drop: %s Nv %u", ITEMS[item].name, (unsigned)game.itemLevel(item));
        dropLine.setColor(Theme::GOOD);
        dropLine.setText(buf);
        sound.playDrop();
    }
    if (game.advDeaths() != seenDeaths)
    {
        seenDeaths = game.advDeaths();
        dropLine.setColor(Theme::BAD);
        dropLine.setText("Voce morreu! Volte quando curar");
        sound.playDeath();
    }

    if (refreshDue(now))
    {
        refresh();
    }
}

void AdventureScreen::refresh()
{
    char a[12], b[12], buf[64];
    uint8_t zone = game.zone();
    const ZoneDef *def = game.zoneDef();
    if (zone != shownZone)
    {
        shownZone = zone;
        snprintf(buf, sizeof(buf), "^ %s  (%u/%u) v", def != nullptr ? def->name : "Zona Segura", (unsigned)zone,
                 (unsigned)ZONE_COUNT);
        zoneName.setText(buf);
        sprite.setBackground(def != nullptr ? backdropFor(def->backdrop) : backdropFor(Backdrop::TUTORIAL));
    }

    const EnemyDef *enemy = game.enemy();
    if (enemy != shownEnemy)
    {
        shownEnemy = enemy;
        sprite.setSprite(enemy != nullptr ? spriteFor(enemy->art) : nullptr);
        if (enemy != nullptr)
        {
            sprite.spawn();
        }
    }
    if (enemy != nullptr)
    {
        snprintf(buf, sizeof(buf), game.enemyIsBoss() ? "** %s **" : "%s", enemy->name);
        enemyName.setText(buf);
        enemyName.setColor(game.enemyIsBoss() ? Theme::BAD : Theme::TEXT);
        enemyBar.setValue(game.enemyHp() / game.enemyMaxHp());
        formatNumber(game.enemyHp(), a, sizeof(a));
        formatNumber(game.enemyMaxHp(), b, sizeof(b));
        snprintf(buf, sizeof(buf), "%s / %s", a, b);
        enemyBar.setLabel(buf);
    }
    else
    {
        enemyName.setColor(Theme::TEXT_2);
        enemyName.setText(zone == 0 ? "Descansando (cura 5x)" : "Procurando inimigo...");
        enemyBar.setValue(0);
        enemyBar.setLabel("");
    }
    sprite.setCountdown(zone != 0 && enemy == nullptr ? (uint8_t)ceil(game.respawnLeft()) : 0);

    playerBar.setValue(game.advHp() / game.maxHp());
    formatNumber(game.advHp(), a, sizeof(a));
    formatNumber(game.maxHp(), b, sizeof(b));
    snprintf(buf, sizeof(buf), "%s / %s", a, b);
    playerBar.setLabel(buf);
    strikeBar.setValue(game.strikeReady());
    strikeBar.setLabel(game.strikeReady() >= 1 && enemy != nullptr ? "Pronto: Confirm" : "");

    if (enemy != nullptr)
    {
        formatNumber(game.playerHit(), a, sizeof(a));
        formatNumber(game.enemyHit(), b, sizeof(b));
        snprintf(buf, sizeof(buf), "Voce bate %s  Ele bate %s", a, b);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Cima/Baixo trocam a zona");
    }
    rates.setText(buf);

    formatNumber(game.gold(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "Abates %lu   Ouro %s", (unsigned long)game.advKills(), a);
    tally.setText(buf);
}

void AdventureScreen::stepZone(int8_t delta)
{
    int16_t z = game.zone();
    do
    {
        z += delta;
    } while (z > 0 && z <= ZONE_COUNT && !game.zoneOpen(z));
    if (z < 0 || z > ZONE_COUNT)
    {
        sound.playDenied();
        return;
    }
    game.setZone((uint8_t)z);
    sound.playMenu();
    forceRefresh();
    refresh();
}

void AdventureScreen::onUpPress()
{
    stepZone(1);
}

void AdventureScreen::onDownPress()
{
    stepZone(-1);
}

void AdventureScreen::onConfirmPress()
{
    if (!game.strike())
    {
        sound.playDenied();
    }
}
