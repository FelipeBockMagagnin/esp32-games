#ifndef COLONY_H
#define COLONY_H

#include <Arduino.h>
#include "World.h"

enum class CritterState : uint8_t
{
    WANDER,
    TO_FOOD,
    EATING,
    TO_BED,
    SLEEPING,
    TO_CROP, // Off to bring a ripe crop in to the pantry
    COUNT
};

struct Critter
{
    uint8_t species;
    uint8_t name; // Index into CRITTER_NAMES
    uint8_t x;
    uint8_t y;
    uint8_t state; // CritterState
    uint8_t timer;
    uint16_t hunger; // 0 fed .. NEED_MAX starving
    uint16_t energy; // NEED_MAX rested .. 0
    uint16_t starving; // Ticks spent at full hunger
};

// "sleeping", or "hungry" or "starving!" when that is what matters about it
const char *describeCritter(const Critter &c);

enum class ColonyEvent : uint8_t
{
    NONE,
    BORN,
    LEFT,
};

// The critters: where they walk, what they need, what they earn, and who joins or leaves.
// Each one earns its species' coins scaled by its mood while it is awake.
class Colony
{
public:
    struct Snapshot
    {
        Critter critters[MAX_CRITTERS];
        uint16_t breedTicks;
        uint8_t count;
        uint8_t nextName;
    };

    explicit Colony(World &world);

    void update(unsigned long now);

    uint8_t getCount() const { return count; }
    const Critter &get(uint8_t i) const { return critters[i]; }
    uint8_t countOf(uint8_t species) const;
    uint8_t getCapacity() const;
    bool isHoused() const { return count <= getCapacity(); }
    uint8_t getMood(uint8_t i) const;
    // At the critter's current mood, awake
    uint64_t getCoinsPerMinute(uint8_t i) const;
    uint64_t getTotalCoinsPerMinute() const;

    uint32_t adoptPrice(uint8_t species) const;
    bool canAdopt(uint8_t species) const;
    bool adopt(uint8_t species);
    // Sets it free to make room; nothing is refunded
    void release(uint8_t i);

    // Counts every birth and departure, so screens can diff it
    uint32_t getEventCount() const { return eventCount; }
    ColonyEvent getLastEvent() const { return lastEvent; }
    const char *getLastEventName() const { return CRITTER_NAMES[lastEventName]; }
    uint8_t getLastEventSpecies() const { return lastEventSpecies; }
    // Since boot, not saved
    uint32_t getBirths() const { return births; }
    uint32_t getDepartures() const { return departures; }
    uint32_t getRevision() const { return revision; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    World &world;
    Critter critters[MAX_CRITTERS];
    uint8_t count;
    uint8_t nextName;
    uint16_t breedTicks;
    unsigned long nextTick;

    uint32_t eventCount;
    ColonyEvent lastEvent;
    uint8_t lastEventName;
    uint8_t lastEventSpecies;
    uint32_t births;
    uint32_t departures;
    uint32_t revision;

    void reset();
    void tick();
    void tickCritter(Critter &c, uint8_t index);
    void wander(Critter &c);
    void moveTo(Critter &c, uint8_t x, uint8_t y);
    bool spawn(uint8_t species);
    void remove(uint8_t index);
    void tryBreed();
    void record(ColonyEvent event, const Critter &c);
};

#endif // COLONY_H
