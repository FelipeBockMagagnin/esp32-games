#ifndef PET_ALBUM_H
#define PET_ALBUM_H

#include <stdint.h>
#include "Pet.h"

// What outlives a pet: the forms ever seen, the pets that left an egg behind, the minigame
// records. POD, saved beside the pet.
struct Album
{
    static const uint8_t MEMORY_COUNT = 4;

    struct Memory
    {
        char name[NAME_LEN + 1];
        uint8_t form; // FORM_COUNT for an empty slot
    };

    uint16_t seen; // a bit per FormId
    uint8_t generation; // pets raised to the end before this one
    uint8_t humBest;
    uint8_t catchBest;
    Memory past[MEMORY_COUNT]; // the most recent first

    void clear();
    void see(FormId form) { seen |= (uint16_t)(1u << form); }
    bool hasSeen(FormId form) const { return seen & (1u << form); }
    uint8_t seenCount() const;
    // The pet goes into the memories and a new egg takes its place
    void farewell(Pet &pet);
};

#endif // PET_ALBUM_H
