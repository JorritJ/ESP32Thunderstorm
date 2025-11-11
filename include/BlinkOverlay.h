// --- file: BlinkOverlay.h
#pragma once
#include <Arduino.h>
#include "LedSet.h"

class BlinkOverlay {
public:

    void start(uint32_t now, uint32_t periodMs = 1000, uint8_t dutyPercent = 50,
             float onLevel = 1.0f, float offLevel = 0.0f) {
    enabled = true;
    period = periodMs;
    duty   = constrain(dutyPercent, (uint8_t)1, (uint8_t)99);
    onL    = constrain(onLevel,  0.0f, 1.0f);
    offL   = constrain(offLevel, 0.0f, 1.0f);
    phaseStart = now;
  }

    void stop(LedSet* blinkSet = nullptr) {
        enabled = false;
        if (blinkSet) blinkSet->setAllScaledMasked(0); // zet blink-kanalen uit
    }

    bool isEnabled() const { return enabled; }
    
    // NB: stuur hier de BLINK-set in; NIET de actieve thunder/day-set
    void update(uint32_t now, LedSet& blinkSet) {
        if (!enabled) return;
        if (period < 10) period = 10;

        uint32_t t = (now - phaseStart) % period;
        uint32_t onTime = (uint32_t)((period * (uint32_t)duty) / 100u);
        bool on = (t < onTime);

        // absolute schrijfwijze: kies een duty t.o.v. resolutie van de set
        uint16_t maxv = blinkSet.maxDuty();
        uint16_t dutyVal = (uint16_t)((on ? onL : offL) * maxv);

        // schrijf ALLEEN de kanalen die door de blink-set zijn geselecteerd (w[i]>0)
        blinkSet.setAllScaledMasked(dutyVal);
    }

private:
  bool enabled = false;
  uint32_t period = 1000;
  uint8_t  duty   = 50;     // in percent
  float    onL    = 1.0f;   // 1.0 = vol aan
  float    offL   = 0.0f;   // 0.0 = uit, of bv. 0.3 = dim
  uint32_t phaseStart = 0;
};