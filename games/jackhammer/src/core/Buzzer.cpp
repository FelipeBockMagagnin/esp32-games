#include "Buzzer.h"

void Buzzer::begin(int pin)
{
    ledcSetup(CHANNEL, 2000, 8);
    ledcAttachPin(pin, CHANNEL);
    ledcWriteTone(CHANNEL, 0);
    // Core 0, away from the loop and its SPI pushes
    xTaskCreatePinnedToCore(taskEntry, "buzzer", 2048, this, 2, nullptr, 0);
}

void Buzzer::playEffect(const Note *notes, uint8_t count)
{
    if (count > MAX_EFFECT)
    {
        count = MAX_EFFECT;
    }
    portENTER_CRITICAL(&lock);
    for (uint8_t i = 0; i < count; i++)
    {
        pending[i] = notes[i];
    }
    pendingCount = count;
    effectWaiting = true;
    portEXIT_CRITICAL(&lock);
}

void Buzzer::playSong(const Note *notes, uint16_t count)
{
    portENTER_CRITICAL(&lock);
    nextSong = count > 0 ? notes : nullptr;
    nextSongCount = count;
    songWaiting = true;
    portEXIT_CRITICAL(&lock);
}

void Buzzer::taskEntry(void *self)
{
    ((Buzzer *)self)->run();
}

void Buzzer::run()
{
    for (;;)
    {
        unsigned long now = millis();
        portENTER_CRITICAL(&lock);
        if (effectWaiting)
        {
            effectCount = pendingCount;
            for (uint8_t i = 0; i < effectCount; i++)
            {
                effect[i] = pending[i];
            }
            effectWaiting = false;
            effectPos = 0;
            effectEnd = effectCount > 0 ? now + effect[0].ms : now;
        }
        if (songWaiting)
        {
            song = nextSong;
            songCount = nextSongCount;
            songWaiting = false;
            songPos = 0;
            songEnd = song != nullptr ? now + song[0].ms : now;
        }
        portEXIT_CRITICAL(&lock);

        while (effectPos < effectCount && (long)(now - effectEnd) >= 0)
        {
            effectPos++;
            if (effectPos < effectCount)
            {
                effectEnd += effect[effectPos].ms;
            }
        }
        while (song != nullptr && (long)(now - songEnd) >= 0)
        {
            songPos = (songPos + 1) % songCount;
            songEnd += song[songPos].ms;
        }

        uint16_t freq = 0;
        if (effectPos < effectCount)
        {
            freq = effect[effectPos].freq;
        }
        else if (song != nullptr && (long)(songEnd - now) > GAP_MS)
        {
            freq = song[songPos].freq;
        }
        sound(freq);
        vTaskDelay(1);
    }
}

void Buzzer::sound(uint16_t freq)
{
    if (freq != sounding)
    {
        sounding = freq;
        ledcWriteTone(CHANNEL, freq);
    }
}
