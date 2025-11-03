// --- file: LightProgram.h
#pragma once
#include <Arduino.h>
#include "LedPwm.h"
#include "LedSet.h"

class LightProgram
{
public:
    // Destructor
    virtual ~LightProgram() = default; // virtual destructor for base class
    virtual void start(uint32_t now) = 0; // start the program
    virtual void update(uint32_t now) = 0; // update the program state
};

// ===== ProgThunder =====
/*Klassieke variant*/
/*
class ProgThunder : public LightProgram
{
public:
    ProgThunder(LedPwmChannel &a, LedPwmChannel &b);
    void start(uint32_t now) override;
    void update(uint32_t now) override;

private:
    LedPwmChannel &ch1;
    LedPwmChannel &ch2;
    enum Phase
    {
        Idle,
        PreGlow,
        FlashOn,
        FlashOff,
        AfterGlow
    } phase = Idle;
    
    // Timing
    uint32_t nextEventMs = 0; // moment voor volgende burst
    uint32_t phaseStart = 0; // starttijd van huidige fase
    uint32_t phaseEnd = 0; // eindtijd van huidige fase
    uint32_t chSkewMs = 0; // kanaal 2 vertraging t.o.v. kanaal 1 (8..25 ms)


    // Subflitsconfig
    static constexpr int kMaxSubs = 5;
    int subsTotal = 0; // aantal subflitsen in deze burst
    int subsIndex = 0; // huidige subflits (0..subsTotal-1)
    uint16_t subIntensity[kMaxSubs]; // doelintensiteit per sub (0..maxDuty)


    // Afterglow
    uint16_t afterStartDuty = 0; // startintensiteit voor nagloei


    // Helpers
    static uint32_t randRange(uint32_t a, uint32_t b) { return a + (esp_random() % (b - a + 1)); }
    static float rand01() { return (esp_random() & 0xFFFF) / 65535.0f; }
    void setBoth(uint16_t d1, uint16_t d2) { ch1.setDuty(d1); ch2.setDuty(d2); }
    void setAll(uint16_t d) { setBoth(d, d); }

    void setWithSkew(uint16_t duty, uint32_t now); // ch2 komt iets later op
    void scheduleNextBurst(uint32_t now);
    void prepareBurst(uint32_t now);

    //static uint32_t randRange(uint32_t a, uint32_t b);
    //uint16_t ledMax() const { return ch1.maxDuty(); }
};
*/

// Nieuwe variant met LedSet
class ProgThunder : public LightProgram
{
public:
    ProgThunder(LedSet &set, const float *weights) : leds(set), w(weights) {}    // was: ProgThunder(LedPwmChannel& a, LedPwmChannel& b);
    void start(uint32_t now) override;
    void update(uint32_t now) override;

private:
    LedSet &leds;
    const float *w; // scenario-weights (uit Config)

    enum Phase
    {
        Idle,
        PreGlow,
        FlashOn,
        FlashOff,
        AfterGlow
    } phase = Idle;
    uint32_t nextEventMs = 0, phaseStart = 0, phaseEnd = 0, chSkewMs = 0;
    static constexpr int kMaxSubs = 5;
    int subsTotal = 0, subsIndex = 0;
    uint16_t subIntensity[kMaxSubs];
    uint16_t afterStartDuty = 0;

    static uint32_t randRange(uint32_t a, uint32_t b) { return a + (esp_random() % (b - a + 1)); }
    static float rand01() { return (esp_random() & 0xFFFF) / 65535.0f; }

    void setAll(uint16_t d) { leds.setAllScaled(d); } // <— scaled per scenario-weight
    void setWithSkew(uint16_t duty, uint32_t now);
    void scheduleNextBurst(uint32_t now);
    void prepareBurst(uint32_t now);
};

// ===== ProgDay =====
/*klassieke variant*/
/*
class ProgDay : public LightProgram
{
public:
    ProgDay(LedPwmChannel &a, LedPwmChannel &b);
    void start(uint32_t now) override;
    void update(uint32_t now) override;

private:
    LedPwmChannel &ch1;
    LedPwmChannel &ch2;
    uint32_t nextUpdate = 0;
    float basePhase = 0.f;
    int sparkle1 = 0, sparkle2 = 0;
    static constexpr float TwoPi = 6.28318530718f;
    static uint16_t toDuty(float x);
};
*/
// Nieuwe variant met LedSet
class ProgDay : public LightProgram
{
public:
    ProgDay(LedSet &set, const float *weights) : leds(set), w(weights) {}// was: ProgDay(LedPwmChannel& a, LedPwmChannel& b);
    void start(uint32_t now) override;
    void update(uint32_t now) override;

private:
    LedSet &leds;
    const float* w; // scenario-weights (uit Config)
    uint32_t nextUpdate = 0;
    float basePhase = 0.f;
    int sparkle1 = 0, sparkle2 = 0;
    static constexpr float TwoPi = 6.28318530718f;
    static uint16_t scale(float x, uint16_t maxv)
    {
        x = constrain(x, 0.f, 1.f);
        return (uint16_t)(x * maxv);
    }
};

//static uint16_t toDuty(float x);
