#include "BossScreen.h"
#include "../nan/Format.h"
#include "../ui/Art.h"
#include "../ui/Labels.h"

static const int16_t SPRITE_X = (DISPLAY_W - EnemySprite::SIZE) / 2;
static const int16_t SPRITE_Y = 44;
static const unsigned long NUMBER_MS = 600;

BossScreen::BossScreen(Game &game, SoundManager &sound)
    : Screen("Lutar com Boss"),
      game(game),
      sound(sound),
      name(label(DISPLAY_W / 2, 25, TC_DATUM, Theme::TEXT, 2)),
      spriteFrame(SPRITE_X - 2, SPRITE_Y - 2, EnemySprite::SIZE + 4, EnemySprite::SIZE + 4, false),
      sprite(SPRITE_X, SPRITE_Y),
      bossBar(10, 178, 220, 14, Theme::HP, Theme::HP_HI),
      bossStats(label(DISPLAY_W / 2, 195, TC_DATUM, Theme::TEXT_2)),
      youLabel(label(10, 213)),
      playerBar(42, 210, 188, 14, Theme::BAR, Theme::BAR_HI),
      rates(label(DISPLAY_W / 2, 228, TC_DATUM, Theme::TEXT_2)),
      verdict(label(DISPLAY_W / 2, 240, TC_DATUM)),
      fightButton(60, 254, 120, 24),
      record(label(DISPLAY_W / 2, 284, TC_DATUM, Theme::TEXT_2))
{
    youLabel.setText("Voce");
    fightButton.setFocused(true);
    addElement(&name);
    addElement(&spriteFrame);
    addElement(&sprite);
    addElement(&bossBar);
    addElement(&bossStats);
    addElement(&youLabel);
    addElement(&playerBar);
    addElement(&rates);
    addElement(&verdict);
    addElement(&fightButton);
    addElement(&record);
    addTaskbar();
}

void BossScreen::onEnter(TFT_eSPI &tft)
{
    seenKills = game.bossKills();
    shownBoss = 0xFFFF;
    sprite.stopAnimations();
    hpAtLastNumber = game.bossHp();
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void BossScreen::update(unsigned long now)
{
    sprite.update(now);
    if (game.bossKills() != seenKills)
    {
        seenKills = game.bossKills();
        sprite.kill();
        sound.playBoss();
        hpAtLastNumber = game.bossHp();
    }
    if (wasFighting && !game.isFighting() && game.bossPlayerHp() <= 0)
    {
        sound.playDeath();
    }
    wasFighting = game.isFighting();

    // The damage since the last number, in one number, so a fast fight does not drown the art
    if (game.isFighting() && (long)(now - nextNumberAt) >= 0)
    {
        nextNumberAt = now + NUMBER_MS;
        double dealt = hpAtLastNumber - game.bossHp();
        if (dealt > 0)
        {
            char buf[8];
            formatCompact(dealt, buf, sizeof(buf));
            sprite.addDamage(buf, false);
            sprite.hit(false);
            sound.playHit(false);
        }
        hpAtLastNumber = game.bossHp();
    }

    if (!refreshDue(now))
    {
        return;
    }
    refresh();
}

void BossScreen::refresh()
{
    char a[12], b[12], buf[64];
    uint16_t index = game.bossesBeaten();
    if (index != shownBoss)
    {
        shownBoss = index;
        const BossDef &def = Game::bossDef(index);
        sprite.setSprite(spriteFor(def.art));
        sprite.setBackground(backdropFor(def.backdrop));
        sprite.spawn();
        uint16_t lap = Game::bossLap(index);
        if (lap > 0)
        {
            snprintf(buf, sizeof(buf), "%s+%u", def.name, (unsigned)lap);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%s", def.name);
        }
        name.setText(buf);
    }

    double maxHp = Game::bossMaxHp(index);
    bossBar.setValue(game.bossHp() / maxHp);
    formatNumber(game.bossHp(), a, sizeof(a));
    formatNumber(maxHp, b, sizeof(b));
    snprintf(buf, sizeof(buf), "#%u  %s / %s", (unsigned)index + 1, a, b);
    bossBar.setLabel(buf);
    formatNumber(Game::bossAttack(index), a, sizeof(a));
    formatNumber(Game::bossDefense(index), b, sizeof(b));
    snprintf(buf, sizeof(buf), "ATQ %s  DEF %s", a, b);
    bossStats.setText(buf);

    playerBar.setValue(game.bossPlayerHp() / game.maxHp());
    formatNumber(game.bossPlayerHp(), a, sizeof(a));
    formatNumber(game.maxHp(), b, sizeof(b));
    snprintf(buf, sizeof(buf), "%s / %s", a, b);
    playerBar.setLabel(buf);

    double dps = game.bossDps();
    double hurts = game.bossHurts();
    formatNumber(dps, a, sizeof(a));
    formatNumber(hurts, b, sizeof(b));
    snprintf(buf, sizeof(buf), "Dano/s %s   Sofre/s %s", a, b);
    rates.setText(buf);

    if (dps <= 0)
    {
        verdict.setColor(Theme::BAD);
        verdict.setText("Seu ATQ nao passa a DEF dele");
    }
    else
    {
        double toKill = game.bossHp() / dps;
        double toDie = hurts > 0 ? game.bossPlayerHp() / hurts : 1e18;
        bool wins = toKill < toDie;
        formatTime(toKill < 1 ? 1 : toKill, a, sizeof(a));
        snprintf(buf, sizeof(buf), wins ? "Vence em ~%s" : "Perde: treine mais", a);
        verdict.setColor(wins ? Theme::GOOD : Theme::BAD);
        verdict.setText(buf);
    }

    fightButton.setLabel(game.isFighting() ? "Parar" : "Lutar!");
    snprintf(buf, sizeof(buf), "Derrotados %u   Recorde %u", (unsigned)index, (unsigned)game.bestBossesBeaten());
    record.setText(buf);
}

void BossScreen::onConfirmPress()
{
    if (game.setFighting(!game.isFighting()))
    {
        sound.playMenu();
        hpAtLastNumber = game.bossHp();
    }
    else
    {
        sound.playDenied();
    }
    forceRefresh();
    refresh();
}
