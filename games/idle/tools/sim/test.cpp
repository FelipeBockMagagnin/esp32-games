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
    GameState game{inventory, achievements, ascension};
    CombatState combat{game, inventory};

    void capture(SaveBlob &blob) const
    {
        captureSave(blob, game, inventory, combat, achievements, unlocks, ascension);
    }
    void apply(const SaveBlob &blob)
    {
        applySave(blob, game, inventory, combat, achievements, unlocks, ascension);
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

static void testNextPointSaturates()
{
    puts("next ascension point saturates");
    Ascension ascension;
    Ascension::Snapshot snap = {};
    snap.priorGold = UINT64_MAX - 1;
    ascension.load(snap);
    CHECK(ascension.getRunGoldForNextPoint(0) > 0);
}

int main()
{
    testSaveRoundTrip();
    testSaveSurvivesFailedWrite();
    testUnreadableSlotIsOverwritten();
    testLegacySaveMigrates();
    testZoneHopKeepsHealth();
    testNextPointSaturates();
    printf(failures == 0 ? "all passed\n" : "%d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
