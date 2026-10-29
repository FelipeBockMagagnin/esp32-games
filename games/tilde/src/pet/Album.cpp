#include "Album.h"
#include <string.h>

void Album::clear()
{
    memset(this, 0, sizeof(*this));
    for (uint8_t i = 0; i < MEMORY_COUNT; i++)
    {
        past[i].form = FORM_COUNT;
    }
    see(FORM_EGG);
}

uint8_t Album::seenCount() const
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < FORM_COUNT; i++)
    {
        count += hasSeen((FormId)i) ? 1 : 0;
    }
    return count;
}

void Album::farewell(Pet &pet)
{
    memmove(&past[1], &past[0], sizeof(past[0]) * (MEMORY_COUNT - 1));
    memset(&past[0], 0, sizeof(past[0]));
    memcpy(past[0].name, pet.getName(), strnlen(pet.getName(), NAME_LEN));
    past[0].form = pet.getForm();
    if (generation < 255)
    {
        generation++;
    }
    pet.newEgg();
}
