// --- file: LightProgram.cpp
#include "LightProgram.h"

// ===== ProgThunder =====
//ProgThunder::ProgThunder(LedPwmChannel &a, LedPwmChannel &b) : ch1(a), ch2(b) {}
//ProgThunder::ProgThunder(LedSet &set, const float *weights) : leds(set), w(weights) {}

void ProgThunder::setWithSkew(uint16_t duty, uint32_t now) {
  uint32_t dt = now - phaseStart;
  if (dt < chSkewMs) {
    // korte pre-fase: zet bijvoorbeeld 60% van de intensiteit (geeft het idee dat een deel iets later meekomt)
    setAll((uint16_t)(duty * 0.6f));
  } else {
    setAll(duty);
  }
}

void ProgThunder::scheduleNextBurst(uint32_t now)
{
    // Maak timing minder uniform: vaker kort, soms lang (clustered storms)
    float r = rand01();
    uint32_t shortGap = 800 + (uint32_t)(r * r * 4000); // 0.8..4.8 s met bias naar kort
    if ((esp_random() % 10) == 0)
    {                                      // 10% kans op langere stilte
        shortGap += randRange(3000, 6000); // +3..6 s extra
    }
    nextEventMs = now + shortGap;
}

void ProgThunder::prepareBurst(uint32_t now)
{
    // 1) Kies aantal subflitsen (1..4) en intensiteit per sub
    subsTotal = 1 + (esp_random() % 4);
    subsIndex = 0;
    uint16_t maxv = leds.maxDuty();
    // basisintensiteit 60..100% van max
    float base = 0.60f + 0.40f * rand01();
    for (int i = 0; i < subsTotal; ++i)
    {
        float falloff = 1.0f - 0.15f * i;        // elke sub iets zwakker
        float jitter = 0.90f + 0.20f * rand01(); // ±10%
        float level = base * falloff * jitter;
        if (level > 1.0f)
            level = 1.0f;
        subIntensity[i] = (uint16_t)(level * maxv);
    }

    // 2) Kleine kanaalverschuiving 8..25 ms
    chSkewMs = randRange(8, 25);

    // 3) Start met Preglow naar ~40% van eerste sub
    phase = PreGlow;
    phaseStart = now;
    uint32_t preDur = randRange(10, 40);
    phaseEnd = phaseStart + preDur;
    setAll(0);
}

void ProgThunder::start(uint32_t now)
{
    phase = Idle;
    setAll(0);
    scheduleNextBurst(now);
}

void ProgThunder::update(uint32_t now)
{
    if (now < nextEventMs && phase == Idle)
        return;

    switch (phase)
    {
    case Idle:
        if (now >= nextEventMs)
        {
            prepareBurst(now);
        }
        break;

    case PreGlow:
    {
        // Opbouw naar ~40% van eerste sub
        uint16_t target = (subsTotal > 0) ? (uint16_t)(subIntensity[0] * 0.4f) : 0;
        if (now >= phaseEnd)
        {
            setAll(target);
            // ga naar eerste flits
            phase = FlashOn;
            phaseStart = now;
            uint32_t onDur = randRange(15, 60); // random tijd aan, bereik 15..60 ms
            phaseEnd = phaseStart + onDur;
            setWithSkew(subIntensity[0], now); // direct naar volle intensiteit
        }
        else
        {
            float t = (float)(now - phaseStart) / (float)(phaseEnd - phaseStart);
            uint16_t d = (uint16_t)(t * target);
            setAll(d);
        }
        break;
    }
    case FlashOn:
    {
        // houdt maximale intensiteit voor huidige sub, met kanaal skew
        setWithSkew(subIntensity[subsIndex], now);
        if (now >= phaseEnd)
        {
            // ga kort uit tussen subflitsen
            phase = FlashOff;
            phaseStart = now;
            uint32_t offDur = randRange(30, 120); // random tijd uit,
            phaseEnd = phaseStart + offDur;
            setAll(0);
        }
        break;
    }

    case FlashOff:
    {
        if (now >= phaseEnd)
        {
            ++subsIndex;
            if (subsIndex < subsTotal)
            {
                // volgende subflits
                phase = FlashOn;
                phaseStart = now;
                afterStartDuty = (uint16_t)(subIntensity[subsIndex - 1] * 0.4f); // voor nagloei
                uint32_t glowDur = randRange(10, 50);                           // random tijd aan
                phaseEnd = phaseStart + glowDur;
            }
            else
            {
                // alle subflitsen klaar, naar nagloei
                phase = AfterGlow;
                phaseStart = now;
                afterStartDuty = (uint16_t)(subIntensity[subsTotal - 1] * 0.4f); // startwaarde nagloei
                uint32_t glowDur = randRange(70, 150);                           // random nagloeitijd
                phaseEnd = phaseStart + glowDur;
            }
        }
        break;
    }

    case AfterGlow:
    {
        if (now >= phaseEnd)
        {
            // klaar, terug naar Idle
            phase = Idle;
            setAll(0);
            scheduleNextBurst(now);
        }
        else
        {
            // nagloeien: lineair van afterStartDuty naar 0
            float t = 1.0f - (float)(now - phaseStart) / (float)(phaseEnd - phaseStart);

            if (t < 0)
                t = 0;
            if (t > 1)
                t = 1;
            uint16_t d = (uint16_t)(afterStartDuty * t);
            setAll(d);
        }
        break;
    }
    }
}

// ===== ProgDay =====
//ProgDay::ProgDay(LedPwmChannel &a, LedPwmChannel &b) : ch1(a), ch2(b) {}
//ProgDay::ProgDay(LedSet &set, const float *weights) : leds(set), w(weights) {}

void ProgDay::start(uint32_t now)
{
    basePhase = 0.f;
    nextUpdate = now;
    sparkle1 = 0;
    sparkle2 = 0;
}

/*
uint16_t ProgDay::toDuty(float x)
{
    x = constrain(x, 0.f, 1.f);
    return (uint16_t)(x * ((1u << 12) - 1u));
}
*/

void ProgDay::update(uint32_t now)
{
    if (now < nextUpdate)
        return;
    nextUpdate = now + 10; // ~100 Hz
    basePhase += 0.015f; // speed
    if (basePhase > TwoPi)
        basePhase -= TwoPi;
    float p1 = basePhase; // fase voor eerste LED
    float p2 = basePhase + 1.3f; // offset voor tweede LED

    // Schaal naar dynamische resolutie per kanaal
    //uint16_t max1 = ch1.maxDuty();
    //uint16_t max2 = ch2.maxDuty();
    uint16_t maxv = leds.maxDuty();


    //auto scale = [](float x, uint16_t maxv){ x = constrain(x, 0.f, 1.f); return (uint16_t)(x * maxv); };

    uint16_t d1 = scale(0.35f + 0.25f*(0.5f+0.5f*sinf(p1)), maxv);
    uint16_t d2 = scale(0.35f + 0.25f*(0.5f+0.5f*sinf(p2)), maxv);

    //optionele sparkles
    if((esp_random()&0xFF)==0) sparkle1=1200; if((esp_random()&0xFF)==1) sparkle2=1200;
    if(sparkle1>0){ d1 = (uint16_t)min<uint32_t>(maxv, (uint32_t)d1 + sparkle1); sparkle1-=50; }
    if(sparkle2>0){ d2 = (uint16_t)min<uint32_t>(maxv, (uint32_t)d2 + sparkle2); sparkle2-=50; }
    
    //ch1.setDuty(d1);
    //ch2.setDuty(d2);
    // i.p.v. twee kanalen: neem het gemiddelde of de max als “basis” en laat LedSet schalen per weight
    uint16_t base = max(d1, d2);           // of (d1+d2)/2 voor zachter
    leds.setAllScaled(base);
}