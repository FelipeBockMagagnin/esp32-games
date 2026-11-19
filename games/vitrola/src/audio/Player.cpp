#include "Player.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_timer.h>
#include "Volume.h"

static const uint8_t CHANNEL = 0;
static const uint8_t RESOLUTION = 10;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
#define LOCK() portENTER_CRITICAL(&lock)
#define UNLOCK() portEXIT_CRITICAL(&lock)

static void onTimer(void *self)
{
    ((Player *)self)->tick();
}
#else
#define LOCK()
#define UNLOCK()
#endif

// Pitch bands of the level meter, a third of an octave each from about 100 Hz
static uint8_t bandOf(uint16_t period)
{
    uint32_t hz = PIT_CLOCK / period;
    uint8_t band = 0;
    uint32_t edge = 126;
    while (hz >= edge && band < Player::BANDS - 1)
    {
        band++;
        edge = edge * 5 / 4;
    }
    return band;
}

void Player::begin(int buzzerPin)
{
    pin = buzzerPin;
#ifdef ARDUINO
    duty = volumeDuty(loadVolume());
    ledcSetup(CHANNEL, 1000, RESOLUTION);
    ledcAttachPin(pin, CHANNEL);
    ledcWrite(CHANNEL, 0);
    esp_timer_create_args_t args = {};
    args.callback = onTimer;
    args.arg = this;
    args.name = "player";
    esp_timer_handle_t timer;
    esp_timer_create(&args, &timer);
    // 120 Hz is 8333.3 us; the third of a microsecond loses a frame every 25 s, which no one hears
    esp_timer_start_periodic(timer, 1000000 / FRAME_RATE);
#endif
}

void Player::play(uint16_t index)
{
    if (index >= TRACK_COUNT)
    {
        return;
    }
    LOCK();
    track = index;
    reader.start(TRACKS[index].data, TRACKS[index].size);
    frame = 0;
    loaded = true;
    paused = false;
    ended = false;
    UNLOCK();
}

void Player::setPaused(bool p)
{
    LOCK();
    paused = p && loaded;
    UNLOCK();
}

void Player::stop()
{
    LOCK();
    loaded = false;
    paused = false;
    UNLOCK();
}

bool Player::takeEnded()
{
    LOCK();
    bool was = ended;
    ended = false;
    UNLOCK();
    return was;
}

void Player::tick()
{
    uint16_t next = 0;
    LOCK();
    if (loaded && !paused)
    {
        if (reader.step(next))
        {
            frame = reader.getFrame();
        }
        else
        {
            loaded = false;
            ended = true;
        }
    }
    period = next;
    UNLOCK();

    ticks++;
    for (uint8_t b = 0; b < BANDS; b++)
    {
        if (levels[b] > 0 && (ticks & 1) == 0)
        {
            levels[b] = levels[b] - 1;
        }
    }
    if (next != 0)
    {
        uint8_t b = bandOf(next);
        levels[b] = LEVEL_MAX;
        if (b > 0 && levels[b - 1] < LEVEL_MAX / 2)
        {
            levels[b - 1] = LEVEL_MAX / 2;
        }
        if (b + 1 < BANDS && levels[b + 1] < LEVEL_MAX / 2)
        {
            levels[b + 1] = LEVEL_MAX / 2;
        }
    }
    sound(next);
}

void Player::sound(uint16_t newPeriod)
{
    if (newPeriod == sounding)
    {
        return;
    }
    sounding = newPeriod;
#ifdef ARDUINO
    if (newPeriod == 0)
    {
        ledcWrite(CHANNEL, 0);
        return;
    }
    uint32_t hz = PIT_CLOCK / newPeriod;
    // Below this LEDC cannot divide its clock at 10 bits; an octave up keeps the note
    while (hz < 80)
    {
        hz *= 2;
    }
    ledcChangeFrequency(CHANNEL, hz, RESOLUTION);
    ledcWrite(CHANNEL, duty);
#endif
}
