#include "Clock.h"
#include <WiFi.h>

// Any time before this is the clock's power-on value, not one NTP set
static const time_t SYNCED_AFTER = 1700000000;

void Clock::begin(const char *ssid, const char *password, const char *timezone)
{
    configured = ssid != nullptr && ssid[0] != '\0';
    if (!configured)
    {
        WiFi.mode(WIFI_OFF);
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    lastAttempt = millis();
    // After WiFi.mode(): SNTP needs the TCP/IP stack it brings up, or lwIP asserts
    configTzTime(timezone, "pool.ntp.org", "time.google.com");
}

void Clock::update(unsigned long now)
{
    if (!configured || isConnected() || now - lastAttempt < RETRY_MS)
    {
        return;
    }
    lastAttempt = now;
    WiFi.reconnect();
}

bool Clock::isConnected() const
{
    return configured && WiFi.status() == WL_CONNECTED;
}

bool Clock::isSynced() const
{
    return time(nullptr) > SYNCED_AFTER;
}

bool Clock::getLocal(tm &out) const
{
    if (!isSynced())
    {
        return false;
    }
    time_t t = time(nullptr);
    localtime_r(&t, &out);
    return true;
}

// Howard Hinnant's days_from_civil
static uint32_t daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (uint32_t)(era * 146097 + (int)doe - 719468);
}

uint32_t Clock::today() const
{
    tm local;
    if (!getLocal(local))
    {
        return 0;
    }
    return daysFromCivil(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}
