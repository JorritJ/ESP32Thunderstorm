#pragma once
#include <Arduino.h>

// LED PWM kanaal met instelbare frequentie en resolutie (met defaults)
class LedPwmChannel
{
public:
    // Constructor met optionele frequentie en resolutie
    LedPwmChannel(int channel, int pin,
                  uint32_t freq = 3000,
                  uint8_t resBits = 12);

    // Initialiseer het kanaal
    void begin();
    // Stel de duty cycle in (0..maxDuty())
    void setDuty(uint16_t duty);

    //Getter methods
    uint16_t maxDuty() const { return (1u << resBits) - 1u; }
    int channel() const { return ch; }
    int pinNumber() const { return pin; }
    uint32_t frequency() const { return freq; }
    uint8_t resolutionBits() const { return resBits; }

    // runtime aanpassingen (herconfigureert ledcSetup)
    void setFrequency(uint32_t newFreq);
    void setResolution(uint8_t newResBits);

private:
    int ch;
    int pin;
    uint32_t freq; //Hz
    uint8_t resBits; //bits
};