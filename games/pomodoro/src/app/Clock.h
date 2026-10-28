#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <time.h>

// Wall time from NTP over WiFi, since the board has no RTC. Nothing here blocks: the WiFi
// joins in the background and SNTP sets the system clock once it can.
class Clock
{
public:
    // An empty ssid leaves the WiFi off, and the clock never syncs
    void begin(const char *ssid, const char *password, const char *timezone);
    void update(unsigned long now);

    bool hasWifiConfig() const { return configured; }
    bool isConnected() const;
    bool isSynced() const;
    // False until synced
    bool getLocal(tm &out) const;
    // Days since 1970-01-01 in local time, 0 until synced
    uint32_t today() const;

private:
    static const unsigned long RETRY_MS = 30000;

    bool configured = false;
    unsigned long lastAttempt = 0;
};

// 0 is Sunday, like tm_wday
inline uint8_t weekdayOf(uint32_t day)
{
    return (uint8_t)((day + 4) % 7); // 1970-01-01 was a Thursday
}

#endif // CLOCK_H
