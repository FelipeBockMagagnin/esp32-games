// An in-memory NVS for the tests: one store shared by every Preferences, so a save written
// by one is read by the next, and a switch to make writes fail like a power cut would.
#ifndef SIM_PREFERENCES_H
#define SIM_PREFERENCES_H

#include <Arduino.h>
#include <map>
#include <vector>

class Preferences
{
public:
    static inline std::map<std::string, std::vector<uint8_t>> store;
    static inline bool failWrites = false;

    bool begin(const char *, bool readOnly = false)
    {
        this->readOnly = readOnly;
        return true;
    }
    void end() {}

    size_t putBytes(const char *key, const void *value, size_t len)
    {
        if (readOnly || failWrites)
        {
            return 0;
        }
        const uint8_t *bytes = (const uint8_t *)value;
        store[key].assign(bytes, bytes + len);
        return len;
    }
    size_t putUInt(const char *key, uint32_t value) { return putBytes(key, &value, sizeof(value)); }

    bool isKey(const char *key) { return store.count(key) > 0; }
    size_t getBytesLength(const char *key) { return isKey(key) ? store[key].size() : 0; }
    size_t getBytes(const char *key, void *buf, size_t maxLen)
    {
        if (!isKey(key) || store[key].size() > maxLen)
        {
            return 0;
        }
        memcpy(buf, store[key].data(), store[key].size());
        return store[key].size();
    }
    uint32_t getUInt(const char *key, uint32_t defaultValue = 0)
    {
        uint32_t value = defaultValue;
        return getBytesLength(key) == sizeof(value) && getBytes(key, &value, sizeof(value)) ? value : defaultValue;
    }

    bool remove(const char *key) { return readOnly ? false : store.erase(key) > 0; }
    bool clear()
    {
        store.clear();
        return true;
    }

private:
    bool readOnly = false;
};

#endif // SIM_PREFERENCES_H
