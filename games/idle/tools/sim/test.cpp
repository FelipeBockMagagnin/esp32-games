// Checks on the game logic that the balance simulator cannot see: the save slots, the
// combat rules around zone changes, and the late-game saturation. Build and run with
// tools/sim/test.sh; exits non-zero on the first failure.
#include <Arduino.h>
#include <Preferences.h>
#include <random>

#include "../../src/game/GameState.h"
#include "../../src/game/Inventory.h"
#include "../../src/game/CombatState.h"
#include "../../src/game/Achievements.h"
#include "../../src/game/Unlocks.h"
#include "../../src/game/Ascension.h"
#include "../../src/game/SaveGame.h"
#include "../../src/game/Format.h"
#include "../../src/game/ItemStats.h"

static unsigned long simNow = 0;
static std::mt19937 rng(1);
unsigned long millis() { return simNow; }
long random(long max) { return max <= 0 ? 0 : (long)(rng() % (unsigned long)max); }
long random(long min, long max) { return min + random(max - min); }

static int failures = 0;

#define CHECK(cond)                                                       \
    do                                                                    \
    {                                                                     \
        if (!(cond))                                                      \
        {                                                                 \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            failures++;                                                   \
        }                                                                 \
    } while (0)

// A whole game, built in dependency order like main.cpp's globals
struct World
{
    Achievements achievements;
    Unlocks unlocks;
    Inventory inventory;
    Ascension ascension;
    Journal journal;
    GameState game{inventory, achievements, ascension};
    CombatState combat{game, inventory};

    void capture(SaveBlob &blob) const
    {
        captureSave(blob, game, inventory, combat, achievements, unlocks, ascension, journal);
    }
    void apply(const SaveBlob &blob)
    {
        applySave(blob, game, inventory, combat, achievements, unlocks, ascension, journal);
    }
};

static void tick(World &w, unsigned long ms)
{
    for (unsigned long end = simNow + ms; simNow < end; simNow += 100)
    {
        w.game.update(simNow);
        w.combat.update(simNow);
    }
}

static bool sameBlob(const SaveBlob &a, const SaveBlob &b)
{
    return memcmp(&a, &b, sizeof(SaveBlob)) == 0;
}

static void corruptSlotHolding(const SaveBlob &blob)
{
    for (auto &entry : Preferences::store)
    {
        if (entry.second.size() > sizeof(SaveBlob) && memcmp(entry.second.data(), &blob, sizeof(SaveBlob)) == 0)
        {
            entry.second[sizeof(SaveBlob) / 2] ^= 0xFF;
        }
    }
}

static void testSaveRoundTrip()
{
    puts("save round trip");
    Preferences::store.clear();
    eraseSave();

    World a;
    a.game.addGold(1234);
    a.game.buyBuilding(0);
    SaveBlob written;
    a.capture(written);
    CHECK(saveGame(written));

    SaveBlob read;
    CHECK(loadGame(read));
    CHECK(sameBlob(read, written));

    World b;
    b.apply(read);
    CHECK(b.game.getGold() == a.game.getGold());
    CHECK(b.game.getBuildingLevel(0) == 1);
}

// A failed write must leave the previous save loadable, and the one after it must not
// land on that same good copy
static void testSaveSurvivesFailedWrite()
{
    puts("save survives a failed write");
    Preferences::store.clear();
    eraseSave();

    World w;
    SaveBlob first, second, read;
    w.game.addGold(10);
    w.capture(first);
    CHECK(saveGame(first));

    w.game.addGold(10);
    w.capture(second);
    Preferences::failWrites = true;
    CHECK(!saveGame(second));
    Preferences::failWrites = false;
    CHECK(loadGame(read));
    CHECK(sameBlob(read, first));

    // Corrupting the newest slot falls back to the one before
    CHECK(saveGame(second));
    CHECK(loadGame(read));
    CHECK(sameBlob(read, second));
    corruptSlotHolding(second);
    CHECK(loadGame(read));
    CHECK(sameBlob(read, first));
}

// A newer slot this firmware cannot read (another SAVE_VERSION) is the one overwritten,
// never the readable one
static void testUnreadableSlotIsOverwritten()
{
    puts("unreadable slot is the one overwritten");
    Preferences::store.clear();
    eraseSave();

    World w;
    SaveBlob good, newer, read;
    w.capture(good);
    CHECK(saveGame(good));
    newer = good;
    newer.version = SAVE_VERSION + 1;
    CHECK(saveGame(newer));

    CHECK(loadGame(read));
    CHECK(sameBlob(read, good));
    w.game.addGold(5);
    SaveBlob next;
    w.capture(next);
    CHECK(saveGame(next));
    CHECK(loadGame(read));
    CHECK(sameBlob(read, next));

    // The good copy is still there behind it
    corruptSlotHolding(next);
    CHECK(loadGame(read));
    CHECK(sameBlob(read, good));
}

static void testLegacySaveMigrates()
{
    puts("legacy single-slot save loads and is removed");
    Preferences::store.clear();
    eraseSave();

    World w;
    w.game.addGold(77);
    SaveBlob legacy;
    w.capture(legacy);
    // The old format: the blob, and FNV-1a over it under its own key
    uint32_t hash = 2166136261u;
    const uint8_t *bytes = (const uint8_t *)&legacy;
    for (size_t i = 0; i < sizeof(legacy); i++)
    {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    Preferences prefs;
    prefs.begin("idlegame");
    prefs.putBytes("blob", &legacy, sizeof(legacy));
    prefs.putUInt("crc", hash);

    SaveBlob read;
    CHECK(loadGame(read));
    CHECK(sameBlob(read, legacy));
    CHECK(saveGame(read));
    CHECK(!prefs.isKey("blob"));
    CHECK(!prefs.isKey("crc"));
    CHECK(loadGame(read));
    CHECK(sameBlob(read, legacy));
}

// A save from before the Palette talent: same bytes, but fingerprinted with one talent less
static void testSaveBeforePaletteLoads()
{
    puts("a save from before the Palette talent loads with Palette at 0");
    Preferences::store.clear();
    eraseSave();

    World w;
    w.game.addGold(55);
    SaveBlob old;
    w.capture(old);
    const uint32_t sizes[] = {
        (uint32_t)sizeof(SaveBlob), BUILDING_COUNT, GOLD_UPGRADE_COUNT, ITEM_COUNT,
        EQUIP_SLOT_COUNT, ZONE_COUNT, ACHIEVEMENT_COUNT, FEATURE_COUNT, (uint32_t)TALENT_COUNT - 1};
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < sizeof(sizes); i++)
    {
        hash = (hash ^ ((const uint8_t *)sizes)[i]) * 16777619u;
    }
    old.layout = hash;
    hash = 2166136261u;
    for (size_t i = 0; i < sizeof(old); i++)
    {
        hash = (hash ^ ((const uint8_t *)&old)[i]) * 16777619u;
    }
    Preferences prefs;
    prefs.begin("idlegame");
    prefs.putBytes("blob", &old, sizeof(old));
    prefs.putUInt("crc", hash);

    SaveBlob read;
    CHECK(loadGame(read));
    CHECK(read.game.gold == old.game.gold);
    CHECK(read.ascension.talentLevels[TALENT_PALETTE] == 0);
}

static uint32_t fnv(const void *data, size_t len, uint32_t hash = 2166136261u)
{
    for (size_t i = 0; i < len; i++)
    {
        hash = (hash ^ ((const uint8_t *)data)[i]) * 16777619u;
    }
    return hash;
}

// A version 10 slot is today's blob cut before the journal, with its own size in the layout
static void testSaveBeforeJournalLoads()
{
    puts("a save from before the Journal loads with empty records");
    Preferences::store.clear();
    eraseSave();

    World w;
    w.game.addGold(77);
    SaveBlob blob;
    w.capture(blob);
    const size_t oldSize = (offsetof(SaveBlob, journal) + 7) / 8 * 8;
    blob.version = 10;
    const uint32_t sizes[] = {
        (uint32_t)oldSize, BUILDING_COUNT, GOLD_UPGRADE_COUNT, ITEM_COUNT,
        EQUIP_SLOT_COUNT, ZONE_COUNT, ACHIEVEMENT_COUNT, FEATURE_COUNT, (uint32_t)TALENT_COUNT};
    blob.layout = fnv(sizes, sizeof(sizes));

    std::vector<uint8_t> slot(oldSize + 8, 0);
    memcpy(slot.data(), &blob, offsetof(SaveBlob, journal));
    uint32_t sequence = 3;
    memcpy(slot.data() + oldSize, &sequence, 4);
    uint32_t crc = fnv(slot.data(), oldSize + 4);
    memcpy(slot.data() + oldSize + 4, &crc, 4);
    Preferences prefs;
    prefs.begin("idlegame");
    prefs.putBytes("save0", slot.data(), slot.size());

    SaveBlob read;
    CHECK(loadGame(read));
    CHECK(read.version == SAVE_VERSION);
    CHECK(read.game.gold == 77);
    CHECK(read.journal.runCount == 0 && read.journal.pastClicks == 0);

    // The next write goes to the other slot, a full-size one
    CHECK(saveGame(read));
    SaveBlob again;
    CHECK(loadGame(again));
    CHECK(sameBlob(again, read));
}

static void testJournalBanksARun()
{
    puts("the journal counts play time and banks a run's totals");
    World w;
    bool leveled;
    w.journal.update(1000, w.game, w.combat);
    for (unsigned long t = 1000; t < 1500; t += 100)
    {
        w.game.mine(leveled, t);
    }
    w.journal.update(4600, w.game, w.combat);
    CHECK(w.journal.getRunSeconds() == 3);
    CHECK(w.journal.getBestStreak() == 5);

    w.journal.bankRun(w.game, w.combat, 7);
    CHECK(w.journal.getRunSeconds() == 0);
    CHECK(w.journal.getTotalSeconds() == 3);
    CHECK(w.journal.getRunCount() == 1);
    CHECK(w.journal.getRun(0).points == 7 && w.journal.getRun(0).seconds == 3);
    CHECK(w.journal.getLifetimeClicks(w.game) == 10); // Banked 5, and the run still holds its 5
}

// Settings saved before the theme field existed keep their values and read theme 0
static void testShortSettingsLoad()
{
    puts("settings from before a new field still load");
    Preferences::store.clear();
    Preferences prefs;
    prefs.begin("idlegame");
    const uint8_t old[2] = {2, 0};
    prefs.putBytes("settings", old, sizeof(old));

    Settings read = {9, 9, 9};
    CHECK(loadSettings(read));
    CHECK(read.sound == 2 && read.dimAfter == 0 && read.theme == 0);
}

// Hopping between zones must not refill health nor cut a revive short
static void testZoneHopKeepsHealth()
{
    puts("zone hop keeps health and death");
    World w;
    simNow = 1000;
    CHECK(w.combat.enterZone(0, simNow));
    CHECK(w.combat.getPlayerHp() == w.combat.getPlayerStats().maxHp);

    for (int i = 0; i < 600 && w.combat.getPlayerHp() == w.combat.getPlayerStats().maxHp; i++)
    {
        tick(w, 1000);
    }
    uint32_t hurt = w.combat.getPlayerHp();
    CHECK(hurt < w.combat.getPlayerStats().maxHp);
    CHECK(w.combat.enterZone(0, simNow));
    CHECK(w.combat.getPlayerHp() == hurt);

    // Bare-handed the first zone wins, so the player eventually goes down
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        w.inventory.unequip((EquipSlot)slot);
    }
    for (int i = 0; i < 3600 && w.combat.getPhase() != CombatState::DEAD; i++)
    {
        tick(w, 1000);
    }
    if (w.combat.getPhase() != CombatState::DEAD)
    {
        puts("  (skipped the death half: the first zone never won)");
        return;
    }
    unsigned long revive = w.combat.getReviveRemaining(simNow);
    CHECK(w.combat.enterZone(0, simNow));
    CHECK(w.combat.getPhase() == CombatState::DEAD);
    CHECK(w.combat.getReviveRemaining(simNow) == revive);
}

static void testNewItemDropsMoreOften()
{
    puts("an unowned item drops more often, up to certain");
    World w;
    DropDef drop = {ITEM_EMBER_HELM, 20};
    uint16_t unowned = w.combat.getDropChanceBp(drop);
    w.inventory.addDrop(ITEM_EMBER_HELM);
    CHECK(unowned == w.combat.getDropChanceBp(drop) * NEW_ITEM_DROP_MULTIPLIER);
    CHECK(w.combat.getDropChanceBp({ITEM_EMBER_MAIL, 9000}) == 10000);
}

static void testNextPointSaturates()
{
    puts("next ascension point saturates");
    Ascension ascension;
    Ascension::Snapshot snap = {};
    snap.priorUnits = UINT64_MAX - 1;
    ascension.load(snap);
    CHECK(ascension.getRunGoldForNextPoint(0) > 0);
}

static void testGoldPastThousandthsCap()
{
    puts("gold goes past 1.8e16 and saturates at the top");
    World w;
    const uint64_t oldCap = UINT64_MAX / GOLD_SCALE;
    w.game.addGold(oldCap);
    w.game.addGold(oldCap);
    CHECK(w.game.getGold() == oldCap * 2);
    CHECK(w.game.getTotalGoldEarned() == oldCap * 2);
    w.game.addGold(UINT64_MAX);
    CHECK(w.game.getGold() == UINT64_MAX);
    CHECK(w.game.getTotalGoldEarned() == UINT64_MAX);
}

static void testThousandthsSaveMigrates()
{
    puts("a version 7 save, gold in thousandths, loads in whole units");
    Preferences::store.clear();
    eraseSave();

    World w;
    SaveBlob old, read;
    w.capture(old);
    old.version = 7;
    old.game.gold = 77 * GOLD_SCALE + 500;
    old.game.totalGoldEarned = 90 * GOLD_SCALE;
    CHECK(saveGame(old));

    CHECK(loadGame(read));
    CHECK(read.version == SAVE_VERSION);
    CHECK(read.game.gold == 77);
    CHECK(read.game.totalGoldEarned == 90);
}

static void testCompactFormat()
{
    puts("stat card numbers stay within 4 chars");
    CHECK(strcmp(formatCompact(9999).c_str(), "9999") == 0);
    CHECK(strcmp(formatCompact(10000).c_str(), "10K") == 0);
    CHECK(strcmp(formatCompact(24464).c_str(), "24K") == 0);
    CHECK(strcmp(formatCompact(999999).c_str(), "999K") == 0);
    CHECK(strcmp(formatCompact(1250000).c_str(), "1.2M") == 0);
    CHECK(strcmp(formatAmount(UINT64_MAX).c_str(), "18.4Qi") == 0);
    for (uint64_t v = 1; v <= UINT32_MAX; v = v * 3 + 1)
    {
        CHECK(strlen(formatCompact((uint32_t)v).c_str()) <= 4);
    }
    CHECK(strlen(formatCompact(UINT32_MAX).c_str()) <= 4);
}

static int16_t upgradeIndex(const char *name)
{
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (strcmp(GOLD_UPGRADES[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// With every screen open, the bar names the next building, then the next zone
static void testLateGoals()
{
    puts("late goals point at the next building and zone");
    World w;
    Unlocks::Snapshot open = {};
    for (uint8_t i = 0; i < FEATURE_COUNT; i++)
    {
        open.unlocked[i] = true;
    }
    w.unlocks.load(open);

    GameState::Snapshot game = {};
    game.miningLevel = 1;
    for (uint8_t i = 0; i + 1 < BUILDING_COUNT; i++)
    {
        game.buildingLevels[i] = 1;
    }
    game.gold = BUILDINGS[BUILDING_STAR_MINE].baseCost / 2;
    w.game.load(game);
    simNow += 1000;
    w.unlocks.evaluate(simNow, w.game, w.inventory, w.combat, w.achievements, false);
    const Unlocks::Goal *goal = w.unlocks.getGoal();
    CHECK(goal != nullptr);
    CHECK(strcmp(goal->label, "Buy your first Star Mine") == 0);
    // Past 32 bits the progress is in thousandths
    CHECK(goal->target == 1000);
    CHECK(goal->done == 500);

    game.buildingLevels[BUILDING_STAR_MINE] = 1;
    w.game.load(game);
    simNow += 1000;
    w.unlocks.evaluate(simNow, w.game, w.inventory, w.combat, w.achievements, false);
    CHECK(strcmp(w.unlocks.getGoal()->label, "Buy Forest Maps to open Forest") == 0);

    game.goldUpgradesBought[upgradeIndex("Forest Maps")] = true;
    w.game.load(game);
    CombatState::Snapshot combat = {};
    combat.zoneKills[0] = 10;
    w.combat.load(combat);
    simNow += 1000;
    w.unlocks.evaluate(simNow, w.game, w.inventory, w.combat, w.achievements, false);
    goal = w.unlocks.getGoal();
    CHECK(strcmp(goal->label, "Fight in Sewers to open Forest") == 0);
    CHECK(goal->done == 10 && goal->target == ZONES[1].reqPrevZoneKills);
}

static void testAwardProduction()
{
    puts("award upgrades pay per achievement unlocked");
    World w;
    GameState::Snapshot game = {};
    game.miningLevel = 1;
    game.goldUpgradesBought[upgradeIndex("Trophy Shelf")] = true;
    w.game.load(game);
    uint32_t before = w.game.getProductionBonusPercent();

    Achievements::Snapshot awards = {};
    for (uint8_t i = 0; i < 10; i++)
    {
        awards.unlocked[i] = true;
    }
    w.achievements.load(awards);
    uint32_t awardsOwn = w.achievements.getProductionBonusPercent();
    CHECK(w.game.getProductionBonusPercent() - before == 10 + awardsOwn);
}

// Dull Stick: attack 5 reaches 6 at Lv 15, click gold 8% reaches 9% at Lv 10 and 10% at Lv 18
static void testNextGain()
{
    puts("next gain points at the soonest-rising stat");
    const ItemDef &stick = ITEMS[ITEM_DULL_STICK];
    NextGain gain = findNextGain(stick, 1);
    CHECK(gain.found && gain.kind == STAT_CLICK && gain.levels == 9 && gain.sinceLevel == 1 && gain.change == 1);
    gain = findNextGain(stick, 10);
    CHECK(gain.found && gain.kind == STAT_ATTACK && gain.levels == 5 && gain.sinceLevel == 1);
    gain = findNextGain(stick, 15);
    CHECK(gain.found && gain.kind == STAT_CLICK && gain.levels == 3 && gain.sinceLevel == 10);
    CHECK(!findNextGain(stick, ITEM_MAX_LEVEL).found);

    char buf[32];
    formatNextGain(buf, sizeof(buf), findNextGain(stick, 1), STAT_LABELS);
    CHECK(strcmp(buf, "Click gold +1% in 9 lv") == 0);
}

// Awards that came before Upgrade opened do not open Awards right behind it
static void testAwardsWaitForANewAward()
{
    puts("awards opens on an award earned after upgrade");
    World w;
    Unlocks::Snapshot open = {};
    open.unlocked[(uint8_t)Feature::BUILDINGS] = true;
    open.unlocked[(uint8_t)Feature::UPGRADES] = true;
    w.unlocks.load(open);

    Achievements::Snapshot awards = {};
    awards.unlocked[0] = true;
    awards.unlocked[1] = true;
    w.achievements.load(awards);

    GameState::Snapshot game = {};
    game.miningLevel = 1;
    w.game.load(game);
    for (int i = 0; i < 20; i++)
    {
        simNow += 1000;
        w.unlocks.evaluate(simNow, w.game, w.inventory, w.combat, w.achievements, false);
    }
    CHECK(!w.unlocks.isUnlocked(Feature::AWARDS));

    awards.unlocked[2] = true;
    w.achievements.load(awards);
    simNow += 1000;
    w.unlocks.evaluate(simNow, w.game, w.inventory, w.combat, w.achievements, false);
    CHECK(w.unlocks.isUnlocked(Feature::AWARDS));
}

// Waits for a fight with the strike ready, then strikes
static bool strikeWhenReady(World &w)
{
    for (int i = 0; i < 600; i++)
    {
        if (w.combat.getPhase() == CombatState::FIGHTING && w.combat.getStrikeCooldownRemaining(simNow) == 0)
        {
            return w.combat.strike(simNow);
        }
        tick(w, 100);
    }
    return false;
}

static void testArmorBreakLastsOneFight()
{
    puts("armor break stacks per strike, resets on a new enemy");
    World w;
    simNow = 1000;
    w.inventory.addDrop(ITEM_RAT_FANG);
    w.inventory.equip(ITEM_RAT_FANG);
    CHECK(w.combat.enterZone(0, simNow));
    CHECK(strikeWhenReady(w));
    if (w.combat.getPhase() != CombatState::FIGHTING)
    {
        puts("  (skipped: the first strike killed)");
        return;
    }
    uint8_t broken = w.combat.getArmorBreakPercent();
    CHECK(broken == w.combat.getArmorBreakPerk());
    CHECK(w.combat.getEnemyDefense() < w.combat.getEnemy().defense);
    // A strike refused on its cooldown breaks nothing
    CHECK(!w.combat.strike(simNow));
    CHECK(w.combat.getArmorBreakPercent() == broken);
    w.combat.rerollEnemy(simNow);
    CHECK(w.combat.getArmorBreakPercent() == 0);
    CHECK(w.combat.getEnemyDefense() == w.combat.getEnemy().defense);
}

static void testWeakPointCrits()
{
    puts("a strike through an open weak point crits and closes it");
    World w;
    simNow = 1000;
    w.inventory.addDrop(ITEM_HAWK_GLOVES);
    w.inventory.equip(ITEM_HAWK_GLOVES);
    CHECK(w.combat.enterZone(0, simNow));
    for (int i = 0; i < 2000 && !(w.combat.isWeakPointOpen(simNow) && w.combat.getStrikeCooldownRemaining(simNow) == 0 &&
                                  w.combat.getPhase() == CombatState::FIGHTING); i++)
    {
        tick(w, 100);
    }
    CHECK(w.combat.isWeakPointOpen(simNow));
    uint32_t plain = w.combat.getStrikeDamage();
    CHECK(w.combat.strike(simNow));
    CHECK(w.combat.getLastHitKind() == CombatState::HIT_CRIT);
    CHECK(w.combat.getLastHitDamage() == plain * (100 + w.combat.getWeakPointPerk()) / 100);
    CHECK(!w.combat.isWeakPointOpen(simNow));
}

static void testFuryNeedsItsPerk()
{
    puts("fury fills only with its perk and quickens the strike");
    World plain;
    simNow = 1000;
    CHECK(plain.combat.enterZone(0, simNow));
    for (int i = 0; i < 8; i++)
    {
        strikeWhenReady(plain);
    }
    CHECK(plain.combat.getFuryPercent() == 0);
    CHECK(plain.combat.getFuryEvents() == 0);

    World w;
    w.inventory.addDrop(ITEM_RAGE_MASK);
    w.inventory.equip(ITEM_RAGE_MASK);
    CHECK(w.combat.enterZone(0, simNow));
    for (int i = 0; i < 20 && !w.combat.isFuryActive(simNow); i++)
    {
        strikeWhenReady(w);
    }
    CHECK(w.combat.isFuryActive(simNow));
    if (strikeWhenReady(w) && w.combat.isFuryActive(simNow))
    {
        CHECK(w.combat.getStrikeCooldownRemaining(simNow) <= FURY_STRIKE_COOLDOWN_MS);
    }
}

static void testRelicsNeedRelicSense()
{
    puts("relics drop in Relic Sense's order, ten levels a drop");
    World w;
    CHECK(!w.combat.canDrop(ITEM_RAT_FANG));
    CHECK(w.combat.canDrop(ITEM_DULL_STICK));
    Ascension::Snapshot snap = {};
    snap.talentLevels[TALENT_RELIC_SENSE] = 1;
    w.ascension.load(snap);
    CHECK(w.combat.canDrop(ITEM_RAT_FANG));
    CHECK(!w.combat.canDrop(ITEM_HAWK_GLOVES));

    CHECK(w.inventory.addDrop(ITEM_RAT_FANG));
    CHECK(w.inventory.getItemLevel(ITEM_RAT_FANG) == RELIC_DROP_LEVELS);
    for (int i = 0; i < 12; i++)
    {
        CHECK(!w.inventory.addDrop(ITEM_RAT_FANG));
    }
    CHECK(w.inventory.getItemLevel(ITEM_RAT_FANG) == ITEM_MAX_LEVEL);
    w.inventory.addDrop(ITEM_EMBER_BLADE);
    w.inventory.addDrop(ITEM_EMBER_BLADE);
    CHECK(w.inventory.getItemLevel(ITEM_EMBER_BLADE) == 2);
}

int main()
{
    testSaveRoundTrip();
    testSaveSurvivesFailedWrite();
    testUnreadableSlotIsOverwritten();
    testLegacySaveMigrates();
    testShortSettingsLoad();
    testSaveBeforePaletteLoads();
    testSaveBeforeJournalLoads();
    testJournalBanksARun();
    testZoneHopKeepsHealth();
    testNewItemDropsMoreOften();
    testNextPointSaturates();
    testGoldPastThousandthsCap();
    testThousandthsSaveMigrates();
    testCompactFormat();
    testLateGoals();
    testAwardProduction();
    testNextGain();
    testAwardsWaitForANewAward();
    testArmorBreakLastsOneFight();
    testWeakPointCrits();
    testFuryNeedsItsPerk();
    testRelicsNeedRelicSense();
    printf(failures == 0 ? "all passed\n" : "%d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
