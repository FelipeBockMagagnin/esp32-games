// Logic tests on the PC: fairness of every tool, the dice notation, the keeps, the hat, the
// wheel's landing and the saved preferences. Build and run with tools/test.sh.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/dice/History.h"
#include "../src/dice/Prefs.h"
#include "../src/dice/Raffle.h"
#include "../src/dice/Roll.h"
#include "../src/dice/Spinner.h"
#include <Preferences.h>

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0x12345678u;
static uint32_t xorshift()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static const uint32_t *scripted = nullptr;
static uint32_t scriptedNext() { return *scripted++; }

// Pearson's statistic against a uniform spread; with k - 1 degrees of freedom it should stay
// near k, and a biased source lands far past this bound
static void expectUniform(const char *what, const uint32_t *counts, uint32_t k, uint32_t samples)
{
    double expected = (double)samples / k, chi = 0;
    for (uint32_t i = 0; i < k; i++)
    {
        double d = counts[i] - expected;
        chi += d * d / expected;
    }
    double bound = k + 6 * sqrt(2.0 * k);
    printf("  %-22s chi2 %7.1f over %u values (bound %.0f)\n", what, chi, (unsigned)k, bound);
    assert(chi < bound);
}

static void testUniform()
{
    // 2^32 % 6 = 4: the first four raw values are rejected, the fifth taken
    const uint32_t script[] = {0, 1, 2, 3, 4, 11};
    scripted = script;
    assert(uniform(scriptedNext, 6) == 4 % 6);
    scripted = script + 5;
    assert(uniform(scriptedNext, 6) == 11 % 6);
    assert(uniform(xorshift, 1) == 0);
    assert(uniform(xorshift, 0) == 0);

    for (uint32_t sides : {2u, 6u, 20u, 100u})
    {
        static uint32_t counts[100];
        memset(counts, 0, sizeof(counts));
        uint32_t samples = sides * 6000;
        for (uint32_t i = 0; i < samples; i++)
        {
            uint32_t v = uniform(xorshift, sides);
            assert(v < sides);
            counts[v]++;
        }
        char what[24];
        snprintf(what, sizeof(what), "d%u", (unsigned)sides);
        expectUniform(what, counts, sides, samples);
    }
}

static void expectSpec(RollSpec spec, const char *expected)
{
    char buf[24];
    formatSpec(spec, buf, sizeof(buf));
    if (strcmp(buf, expected) != 0)
    {
        printf("formatSpec gave %s, wanted %s\n", buf, expected);
        assert(false);
    }
}

static void testNotation()
{
    expectSpec({1, 6, 0, (uint8_t)Keep::SUM}, "1d20");
    expectSpec({2, 2, 3, (uint8_t)Keep::SUM}, "2d6+3");
    expectSpec({2, 6, 0, (uint8_t)Keep::HIGHEST}, "2d20kh1");
    expectSpec({2, 6, -2, (uint8_t)Keep::LOWEST}, "2d20kl1-2");
    expectSpec({4, 2, 0, (uint8_t)Keep::DROP_LOWEST}, "4d6dl1");
    expectSpec({1, 2, 0, (uint8_t)Keep::DROP_LOWEST}, "1d6");
    expectSpec({10, 7, -20, (uint8_t)Keep::SUM}, "10d100-20");

    RollSpec wild = {0, 99, 127, 9};
    clampSpec(wild);
    assert(wild.count == 1 && wild.die == DEFAULT_DIE && wild.bonus == 0 && wild.keep == (uint8_t)Keep::SUM);
}

static RollResult table(RollSpec spec, std::initializer_list<uint8_t> faces)
{
    RollResult r = {};
    r.spec = spec;
    uint8_t i = 0;
    for (uint8_t f : faces)
    {
        r.faces[i++] = f;
    }
    scoreDice(r);
    return r;
}

static void testKeeps()
{
    char buf[40];
    RollResult r = table({4, 2, 0, (uint8_t)Keep::DROP_LOWEST}, {3, 1, 5, 1});
    assert(r.total == 9 && !r.kept[1] && r.kept[3]);
    formatFaces(r, buf, sizeof(buf));
    assert(strcmp(buf, "3 [1] 5 1") == 0);

    r = table({2, 6, 5, (uint8_t)Keep::HIGHEST}, {7, 20});
    assert(r.total == 25 && r.outcome == Outcome::CRITICAL && !r.kept[0]);
    r = table({2, 6, 0, (uint8_t)Keep::LOWEST}, {1, 20});
    assert(r.total == 1 && r.outcome == Outcome::FUMBLE);
    r = table({2, 6, 0, (uint8_t)Keep::HIGHEST}, {20, 20});
    assert(r.kept[0] && !r.kept[1] && r.outcome == Outcome::CRITICAL);
    // Two d20 summed are no critical, only the best roll there is
    r = table({2, 6, 0, (uint8_t)Keep::SUM}, {20, 20});
    assert(r.total == 40 && r.outcome == Outcome::MAX);
    r = table({3, 2, -1, (uint8_t)Keep::SUM}, {1, 1, 1});
    assert(r.total == 2 && r.outcome == Outcome::MIN);
    r = table({1, 2, 0, (uint8_t)Keep::HIGHEST}, {4});
    assert(r.total == 4 && r.kept[0] && r.outcome == Outcome::PLAIN);

    // The keeps' known means: advantage 13.82, disadvantage 7.18, 4d6 drop lowest 12.24
    const struct
    {
        RollSpec spec;
        double mean;
    } expected[] = {{{2, 6, 0, (uint8_t)Keep::HIGHEST}, 13.825},
                    {{2, 6, 0, (uint8_t)Keep::LOWEST}, 7.175},
                    {{4, 2, 0, (uint8_t)Keep::DROP_LOWEST}, 12.245},
                    {{3, 2, 2, (uint8_t)Keep::SUM}, 12.5}};
    for (auto &e : expected)
    {
        const uint32_t samples = 400000;
        double sum = 0;
        RollResult roll;
        for (uint32_t i = 0; i < samples; i++)
        {
            rollDice(e.spec, xorshift, roll);
            sum += roll.total;
        }
        formatSpec(e.spec, buf, sizeof(buf));
        printf("  %-22s mean %.3f (exact %.3f)\n", buf, sum / samples, e.mean);
        assert(fabs(sum / samples - e.mean) < 0.03);
    }
}

static void testRaffle()
{
    static Raffle raffle;
    raffle.setRepeats(false);
    raffle.setRange(1, 100);
    raffle.restart();
    bool seen[101] = {};
    for (int i = 0; i < 100; i++)
    {
        assert(!raffle.exhausted());
        int16_t v = raffle.draw(xorshift);
        assert(v >= 1 && v <= 100 && !seen[v]);
        seen[v] = true;
    }
    assert(raffle.exhausted() && raffle.remaining() == 0);
    raffle.draw(xorshift);
    assert(raffle.getDrawnCount() == 1);

    raffle.setRange(5, 5);
    assert(raffle.getDrawnCount() == 0 && raffle.draw(xorshift) == 5 && raffle.exhausted());
    raffle.setRange(10, 3);
    assert(raffle.getFrom() == 10 && raffle.getTo() == 10);

    raffle.setRange(RAFFLE_MIN, RAFFLE_MAX);
    static bool all[RAFFLE_MAX + 1];
    memset(all, 0, sizeof(all));
    for (uint32_t i = 0; i < raffle.size(); i++)
    {
        int16_t v = raffle.draw(xorshift);
        assert(!all[v]);
        all[v] = true;
    }
    assert(raffle.exhausted());

    // Which number comes out first must not lean on its place in the range
    static uint32_t firsts[10];
    raffle.setRange(1, 10);
    for (uint32_t i = 0; i < 60000; i++)
    {
        raffle.restart();
        firsts[raffle.draw(xorshift) - 1]++;
    }
    expectUniform("first draw of 1-10", firsts, 10, 60000);

    raffle.setRepeats(true);
    uint32_t repeats = 0;
    int16_t last = -1;
    for (int i = 0; i < 1000; i++)
    {
        int16_t v = raffle.draw(xorshift);
        repeats += v == last;
        last = v;
    }
    assert(repeats > 50 && raffle.remaining() == 10 && !raffle.exhausted());
}

static void testSpinner()
{
    Spinner wheel;
    for (uint8_t slices = MIN_SLICES; slices <= MAX_SLICES; slices++)
    {
        wheel.setSlices(slices);
        static uint32_t wins[MAX_SLICES];
        memset(wins, 0, sizeof(wins));
        const uint32_t spins = 3000;
        unsigned long now = 1000;
        unsigned long longest = 0, shortest = ~0ul;
        for (uint32_t i = 0; i < spins; i++)
        {
            wheel.spin(now, xorshift);
            uint8_t chosen = wheel.getResult();
            longest = wheel.getDuration() > longest ? wheel.getDuration() : longest;
            shortest = wheel.getDuration() < shortest ? wheel.getDuration() : shortest;
            uint16_t lastCrossings = 0;
            // Uneven frames, as a busy loop gives
            while (wheel.isSpinning())
            {
                now += 10 + xorshift() % 40;
                wheel.update(now);
                assert(wheel.getCrossings() >= lastCrossings);
                lastCrossings = wheel.getCrossings();
            }
            assert(wheel.sliceAtPointer() == chosen);
            assert(wheel.getCrossings() >= 4 * slices);
            wins[chosen]++;
            now += 500;
        }
        if (slices == 2 || slices == 6 || slices == 12)
        {
            char what[24];
            snprintf(what, sizeof(what), "wheel of %u", (unsigned)slices);
            expectUniform(what, wins, slices, spins);
            printf("  %-22s spins last %.1f-%.1f s\n", "", shortest / 1000.0, longest / 1000.0);
        }
        assert(shortest > 2500 && longest < 6000);
    }

    // Stopped early, it still lands where it was going
    wheel.setSlices(8);
    wheel.spin(0, xorshift);
    wheel.update(300);
    wheel.stop();
    assert(!wheel.isSpinning() && wheel.sliceAtPointer() == wheel.getResult());
}

static void testHistory()
{
    History history;
    char what[16];
    for (int i = 0; i < 40; i++)
    {
        snprintf(what, sizeof(what), "#%d", i);
        history.add(Tool::ROLL, Mark::PLAIN, (int16_t)i, what, "a detail far longer than the field holds", i);
    }
    assert(history.count() == History::CAPACITY && history.getVersion() == 40);
    assert(history.get(0).result == 39 && strcmp(history.get(0).what, "#39") == 0);
    assert(history.get(History::CAPACITY - 1).result == 40 - History::CAPACITY);
    assert(strlen(history.get(0).detail) == sizeof(history.get(0).detail) - 1);
}

static void testPrefs()
{
    Preferences::store.clear();
    SlotStore store("test");
    Prefs prefs;
    assert(!readPrefs(store, prefs));
    defaultPrefs(prefs);
    prefs.roll = {4, 2, -3, (uint8_t)Keep::DROP_LOWEST};
    prefs.drawFrom = 10;
    prefs.drawTo = 75;
    prefs.drawRepeats = 1;
    prefs.slices = 9;
    assert(writePrefs(store, prefs));

    SlotStore again("test");
    Prefs back;
    defaultPrefs(back);
    assert(readPrefs(again, back));
    assert(back.roll.count == 4 && back.roll.bonus == -3 && back.drawTo == 75 && back.slices == 9);

    prefs.slices = 200;
    prefs.drawTo = 5;
    assert(writePrefs(again, prefs));
    SlotStore third("test");
    assert(readPrefs(third, back) && back.slices == DEFAULT_SLICES && back.drawTo == back.drawFrom);
    assert(sizeof(Prefs) <= SlotStore::MAX_SIZE);
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("uniform\n");
    testUniform();
    printf("notation\n");
    testNotation();
    printf("keeps\n");
    testKeeps();
    printf("raffle\n");
    testRaffle();
    printf("wheel\n");
    testSpinner();
    printf("history\n");
    testHistory();
    printf("prefs\n");
    testPrefs();
    printf("All tests passed\n");
    return 0;
}
