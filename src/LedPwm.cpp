// --- file: LedPwm.cpp
#include "LedPwm.h"

static inline uint16_t clampU16(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return (uint16_t)v;
}

LedPwmChannel::LedPwmChannel(int channel, int pin,
                             uint32_t freq, uint8_t resBits)
    : ch(channel), pin(pin), freq(freq), resBits(resBits) {}

void LedPwmChannel::begin()
{
    ledcSetup(ch, freq, resBits);
    ledcAttachPin(pin, ch);
}

void LedPwmChannel::setDuty(uint16_t duty)
{
    duty = clampU16(duty, 0, maxDuty());
    ledcWrite(ch, duty);
}

void LedPwmChannel::setFrequency(uint32_t newFreq)
{
    freq = newFreq;
    ledcSetup(ch, freq, resBits);
}

void LedPwmChannel::setResolution(uint8_t newResBits)
{
    resBits = newResBits;
    ledcSetup(ch, freq, resBits);
}