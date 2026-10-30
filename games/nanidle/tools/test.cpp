// Logic tests for src/nan/ and the save through SlotStore: tools/test.sh
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "SlotStore.h"
#include "../src/nan/Format.h"
#include "../src/nan/Game.h"

static uint32_t rngState = 7;
static uint32_t rng()
{
    rngState = rngState * 1103515245 + 12345;
    return rngState >> 8;
}
long random(long max) { return max > 0 ? (long)(rng() % max) : 0; }
long random(long min, long max) { return min + random(max - min); }

static void run(Game &game, double seconds)
{
    for (double t = 0; t < seconds; t += 0.1)
    {
        game.tick(0.1);
    }
}

static void testFormat()
{
    char buf[16];
    formatNumber(999, buf, sizeof(buf));
    assert(!strcmp(buf, "999"));
    formatNumber(12345, buf, sizeof(buf));
    assert(!strcmp(buf, "12.3K"));
    formatNumber(4.567e6, buf, sizeof(buf));
    assert(!strcmp(buf, "4.56M"));
    formatNumber(1.5e15, buf, sizeof(buf));
    assert(!strcmp(buf, "1.50Qa"));
    formatNumber(2.5e40, buf, sizeof(buf));
    assert(!strcmp(buf, "2.5e40"));
    formatNumber(NAN, buf, sizeof(buf));
    assert(!strcmp(buf, "NaN"));
    formatCompact(123456, buf, sizeof(buf));
    assert(!strcmp(buf, "123K"));
    formatCompact(4.2e9, buf, sizeof(buf));
    assert(!strcmp(buf, "4.2B"));
    formatTime(3725, buf, sizeof(buf));
    assert(!strcmp(buf, "1h 02m"));
}

static void testEnergy()
{
    Game game(rng);
    run(game, 600);
    assert(game.energy() == game.energyCap());
    assert(game.allocate({false, 0}, 30) == 30);
    assert(game.allocate({false, 1}, 1e9) == game.energyCap() - 30);
    assert(game.idleEnergy() == 0);
    assert(game.allocate({false, 0}, 5) == 0);
    // Locked skills take nothing
    assert(game.allocate({false, 2}, 0) == 0);
    assert(game.reclaim({false, 0}, 100) == 30);
    game.reclaimAll();
    assert(game.allocated() == 0);
}

static void testTraining()
{
    Game game(rng);
    run(game, 60);
    game.allocate({false, 0}, 100);
    // levelCost 50 at power 1: 100 energy makes two levels a second, the overflow kept
    run(game, 10.05);
    assert(game.skillLevel(0) == 20);
    double expected = BASE_ATTACK + 150 * pow(20, SKILL_EXPONENT);
    assert(fabs(game.attack() - expected) < 1e-6);
    assert(game.maxHp() == game.attack() * HP_PER_ATTACK);
    assert(!game.skillOpen(2));
}

static void testBoss()
{
    Game game(rng);
    assert(!game.isUnlocked(FEAT_BOSS));
    run(game, 30);
    game.allocate({false, 0}, 50);
    game.allocate({false, 1}, 50);
    run(game, 300);
    assert(game.isUnlocked(FEAT_BOSS));
    assert(game.bossDps() > 0);
    game.setFighting(true);
    run(game, 120);
    assert(game.bossesBeaten() >= 1);
    assert(game.xp() > 0 || game.xpBought(XpUpgrade::CAP) > 0);

    // A boss whose defense matches the attack takes nothing, without a floor
    Game weak(rng);
    assert(weak.bossDps() == 0);
}

static void testItemsAndEquip()
{
    Game game(rng);
    game.giveItem(0);
    game.giveItem(0);
    assert(game.itemLevel(0) == 2);
    assert(fabs(game.itemPercent(0, true) - ITEMS[0].attack * (1 + ITEM_GROWTH_PER_LEVEL)) < 1e-9);
    double before = game.attack();
    assert(!game.equip((uint8_t)Slot::HEAD, 0)); // A weapon on the head
    assert(game.equip((uint8_t)Slot::WEAPON, 0));
    assert(game.attack() > before);
    // An accessory moves between the two places instead of being worn twice
    game.giveItem(3);
    assert(game.equip(EQUIP_COUNT - 2, 3));
    assert(game.equip(EQUIP_COUNT - 1, 3));
    assert(game.equippedAt(EQUIP_COUNT - 2) == -1 && game.equippedAt(EQUIP_COUNT - 1) == 3);
    for (int i = 0; i < 200; i++)
    {
        game.giveItem(5);
    }
    assert(game.itemLevel(5) == ITEM_MAX_LEVEL);
}

static void testXpAndMinersAndAugments()
{
    Game::Snapshot snap;
    Game game(rng);
    game.capture(snap);
    snap.xp = 1000;
    snap.gold = 1000;
    assert(game.restore(snap));
    double cost = game.xpCost(XpUpgrade::CAP);
    double cap = game.energyCap();
    assert(game.buyXp(XpUpgrade::CAP));
    assert(game.xp() == 1000 - cost);
    assert(game.energyCap() == cap + XP_UPGRADES[0].gain);
    assert(game.xpCost(XpUpgrade::CAP) > cost);

    assert(game.buyMiner(0));
    assert(game.goldPerSecond() == MINERS[0].goldPerSecond);
    assert(game.minerCost(0) == ceil(MINERS[0].cost * MINER_COST_GROWTH));

    // An augment's bar waits full until the gold is there
    run(game, 60);
    game.allocate({true, 0}, game.idleEnergy());
    run(game, 1000);
    assert(game.augmentLevel(0) >= 1);
    assert(game.augmentPercent(AugmentEffect::ATTACK) == AUGMENTS[0].percentPerLevel * game.augmentLevel(0));
}

static void testAdventure()
{
    Game game(rng);
    assert(!game.zoneOpen(1));
    Game::Snapshot snap;
    game.capture(snap);
    snap.bossesBeaten = ZONES[0].reqBosses;
    snap.unlocked = 1u << FEAT_ADVENTURE;
    snap.skillLevel[0] = 3000;
    snap.skillLevel[1] = 3000;
    assert(game.restore(snap));
    assert(game.zoneOpen(1) && !game.zoneOpen(2));
    game.setZone(1);
    assert(game.enemyState() == Game::EnemyState::RESPAWN);
    run(game, 600);
    assert(game.advKills() > 0);
    assert(game.gold() > 0);
    assert(game.isUnlocked(FEAT_MINERS));

    // Too weak for the zone: dies and lands in the safe zone
    Game weak(rng);
    weak.capture(snap);
    snap.bossesBeaten = ZONES[2].reqBosses;
    snap.unlocked = 1u << FEAT_ADVENTURE;
    assert(weak.restore(snap));
    weak.setZone(3);
    run(weak, 60);
    assert(weak.advDeaths() > 0 && weak.zone() == 0);
}

static void testRebirth()
{
    Game game(rng);
    Game::Snapshot snap;
    game.capture(snap);
    snap.bossesBeaten = REBIRTH_MIN_BOSSES + 2;
    snap.skillLevel[0] = 500;
    snap.itemLevel[4] = 3;
    snap.xpBought[0] = 4;
    snap.gold = 1e6;
    assert(game.restore(snap));
    uint32_t points = game.rebirthPoints();
    assert(points == 9);
    game.rebirth();
    assert(game.nan() == points && game.rebirths() == 1);
    assert(game.bossesBeaten() == 0 && game.skillLevel(0) == 0 && game.gold() == 0);
    assert(game.itemLevel(4) == 3 && game.xpBought(XpUpgrade::CAP) == 4);
    assert(game.attack() == BASE_ATTACK * (1 + points * REBIRTH_PERCENT / 100));
    assert(!game.canRebirth());
}

static void testSave()
{
    static_assert(sizeof(Game::Snapshot) <= SlotStore::MAX_SIZE, "the save must fit one SlotStore record");
    Game game(rng);
    run(game, 100);
    game.allocate({false, 0}, 40);
    run(game, 100);
    game.giveItem(2);
    game.equip((uint8_t)Slot::LEGS, 2);

    SlotStore store("nanidle");
    Game::Snapshot snap;
    game.capture(snap);
    assert(store.save(&snap, sizeof(snap)));

    Game::Snapshot loaded;
    SlotStore again("nanidle");
    assert(again.load(&loaded, sizeof(loaded)));
    Game copy(rng);
    assert(copy.restore(loaded));
    assert(copy.skillLevel(0) == game.skillLevel(0));
    assert(copy.equippedAt((uint8_t)Slot::LEGS) == 2);
    assert(fabs(copy.attack() - game.attack()) < 1e-6 * game.attack());

    loaded.version = 99;
    assert(!copy.restore(loaded));
    assert(copy.skillLevel(0) == 0);
    loaded.version = Game::SNAPSHOT_VERSION;
    loaded.equipped[0] = 120;
    assert(!copy.restore(loaded));
}

int main()
{
    testFormat();
    testEnergy();
    testTraining();
    testBoss();
    testItemsAndEquip();
    testXpAndMinersAndAugments();
    testAdventure();
    testRebirth();
    testSave();
    printf("all tests passed (snapshot %zu bytes)\n", sizeof(Game::Snapshot));
}
