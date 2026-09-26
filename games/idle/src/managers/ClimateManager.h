#ifndef CLIMATE_MANAGER_H
#define CLIMATE_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class ClimateManager
{
private:
    // The DHT11 cannot be sampled faster than once per second
    static const unsigned long READ_INTERVAL_MS = 2000;

    DHT dht;
    int pin;
    unsigned long lastMs = 0;
    float temperature = NAN;
    float humidity = NAN;

public:
    ClimateManager(int pin) : dht(pin, DHT11), pin(pin) {}

    void setup()
    {
        dht.begin();
    }

    void loop(unsigned long now)
    {
        if (lastMs != 0 && now - lastMs < READ_INTERVAL_MS)
        {
            return;
        }
        lastMs = now;

        float newTemperature = dht.readTemperature();
        float newHumidity = dht.readHumidity();

        // A failed read returns NaN; the previous sample is kept so the screen doesn't blink
        if (isnan(newTemperature) || isnan(newHumidity))
        {
            Serial.println("DHT11: leitura falhou");
            return;
        }

        temperature = newTemperature;
        humidity = newHumidity;

        Serial.printf("Temperatura: %.1f C | Umidade: %.1f %%\n", temperature, humidity);
    }

    bool hasReading() const { return !isnan(temperature) && !isnan(humidity); }

    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }

    String getTemperatureText() const
    {
        return hasReading() ? String(temperature, 1) + "C" : String("--C");
    }

    String getHumidityText() const
    {
        return hasReading() ? String(humidity, 0) + "%" : String("--%");
    }
};

#endif // CLIMATE_MANAGER_H
