#ifndef SLOT_STORE_H
#define SLOT_STORE_H

#include <Arduino.h>

// One record in NVS, written to two slots in turn, each one key holding the data, a
// sequence number and a checksum. A power cut during a write can only lose that write: the
// other slot still holds the previous one.
class SlotStore
{
public:
    static const size_t MAX_SIZE = 256;

    explicit SlotStore(const char *nameSpace) : nameSpace(nameSpace) {}

    // Both return false on any failure; load also when neither slot holds a record of
    // exactly `len` bytes that passes its checksum
    bool save(const void *data, size_t len);
    bool load(void *data, size_t len);
    // Clears the whole namespace, settings included
    void erase();

    const char *getNamespace() const { return nameSpace; }

private:
    const char *nameSpace;
    uint32_t lastSequence = 0;
    // Always the slot other than the one the live record came from, so a torn write never
    // takes the last good copy with it
    uint8_t nextSlot = 0;
};

#endif // SLOT_STORE_H
