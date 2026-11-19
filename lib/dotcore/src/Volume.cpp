#include "Volume.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *const NAMESPACE = "launcher";
static const char *const KEY_VOLUME = "volume";
static const uint8_t CHANNEL = 0;
static const char *const NAMES[] = {"Mudo", "Baixo", "Medio", "Alto"};
// The fundamental's amplitude goes as sin(pi * duty): about -26 dB, -14 dB and full
static const uint16_t DUTY[] = {0, 12, 64, 512};

struct ToneMsg
{
    uint8_t pin;
    bool start;
    unsigned int hz;
    unsigned long ms;
};

static QueueHandle_t queue = nullptr;
static volatile Volume level = VOLUME_DEFAULT;
static bool levelSet = false;

const char *volumeName(Volume v)
{
    return NAMES[(uint8_t)v < (uint8_t)Volume::COUNT ? (uint8_t)v : (uint8_t)VOLUME_DEFAULT];
}

uint16_t volumeDuty(Volume v)
{
    return DUTY[(uint8_t)v < (uint8_t)Volume::COUNT ? (uint8_t)v : (uint8_t)VOLUME_DEFAULT];
}

Volume loadVolume()
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, true))
    {
        return VOLUME_DEFAULT;
    }
    uint8_t stored = prefs.getUChar(KEY_VOLUME, (uint8_t)VOLUME_DEFAULT);
    prefs.end();
    return stored < (uint8_t)Volume::COUNT ? (Volume)stored : VOLUME_DEFAULT;
}

void saveVolume(Volume v)
{
    Preferences prefs;
    if (prefs.begin(NAMESPACE, false))
    {
        prefs.putUChar(KEY_VOLUME, (uint8_t)v);
        prefs.end();
    }
}

void setVolume(Volume v)
{
    level = v;
    levelSet = true;
}

static void silence(uint8_t pin)
{
    ledcDetachPin(pin);
    ledcWriteTone(CHANNEL, 0);
}

static void toneTask(void *)
{
    ToneMsg msg;
    for (;;)
    {
        xQueueReceive(queue, &msg, portMAX_DELAY);
        if (!msg.start)
        {
            silence(msg.pin);
            continue;
        }
        if (msg.hz == 0)
        {
            silence(msg.pin);
        }
        else
        {
            ledcAttachPin(msg.pin, CHANNEL);
            ledcWriteTone(CHANNEL, msg.hz);
            ledcWrite(CHANNEL, volumeDuty(level));
        }
        if (msg.ms > 0)
        {
            delay(msg.ms);
            silence(msg.pin);
        }
    }
}

static bool begin()
{
    if (queue != nullptr)
    {
        return true;
    }
    if (!levelSet)
    {
        level = loadVolume();
    }
    queue = xQueueCreate(128, sizeof(ToneMsg));
    if (queue == nullptr)
    {
        return false;
    }
    xTaskCreate(toneTask, "toneTask", 3500, nullptr, 1, nullptr);
    return true;
}

void playTone(uint8_t pin, unsigned int hz, unsigned long ms)
{
    if (begin())
    {
        ToneMsg msg = {pin, true, hz, ms};
        xQueueSend(queue, &msg, portMAX_DELAY);
    }
}

void stopTone(uint8_t pin)
{
    if (begin())
    {
        ToneMsg msg = {pin, false, 0, 0};
        xQueueSend(queue, &msg, portMAX_DELAY);
    }
}
