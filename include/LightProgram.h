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
    uint16_t subIntensity[kMaxSubs]; // doelintensiteit per sub (0..maxDuty)
    uint16_t afterStartDuty = 0;

    // per-kanaal offsets voor deze burst
    static constexpr int kMaxCh = 16;        // aantal kanalen, 16 is veilig voor 4 kanalen. Gebruik een grotere waarde als je meer kanalen hebt.
    uint16_t chOffsetMs[kMaxCh] = {0};       // per kanaal random 0..25 ms
    void refreshOffsets(uint16_t intensity); // berekent offsets per subflits
    
    static uint32_t randRange(uint32_t a, uint32_t b) { return a + (esp_random() % (b - a + 1)); } //return random int in [a,b]
    static float rand01() { return (esp_random() & 0xFFFF) / 65535.0f; } //return random float in [0..1]

    // helpers:
    void setAll(uint16_t d) { leds.setAllScaled(d); } // <— scaled per scenario-weight
    void setAllMasked(uint16_t d) { leds.setAllScaledMasked(d); }
    void setOneMasked(int i, uint16_t d) { leds.setOneScaledMasked(i, d); }
    void setWithSkew(uint16_t duty, uint32_t now);
    void scheduleNextBurst(uint32_t now);
    void prepareBurst(uint32_t now);
};

// ===== ProgDay =====
class ProgDay : public LightProgram
{
public:
    ProgDay(LedSet &set, const float *weights) : leds(set), w(weights) {}
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
    void setAll(uint16_t d) { leds.setAllScaled(d); } // <— scaled per scenario-weight
    inline void setAllMasked(uint16_t d) { leds.setAllScaledMasked(d); }
};

//static uint16_t toDuty(float x);
