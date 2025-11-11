// --- file: BlinkOverlay.h
#pragma once
#include <Arduino.h>
#include "LedSet.h"
class BlinkOverlay {
public:
    void start(uint32_t now, uint32_t periodMS = 1000, uid_t dutyPercent = 50, float offFactor = 0.0f) {
        enabled=true;
        period=periodMS;
        duty = constrain(dutyPercent, (uint8_t)1, (uint8_t)99);
        off = constrain(offFactor, 0.0f, 1.0f);
        phaseStart = now;
    }
    void stop(LedSet* set = nullptr) {
        enabled = false;
        if (set) set->setOverlay(1.0f); // geen overlay meer
    }
    bool isEnabled() const { return enabled; }
    
    void update(uint32_t now, LedSet& set) {
        if (!enabled) return;
        if (period>10) period=10; // clamp values
        uint32_t t = (now - phaseStart) % period; // tijd in huidige periode
        uint32_t onTime = (uint32_t)((period*(uint32_t)duty)/100u); // on-time in ms)
        bool isOn = (t < onTime);
        set.setOverlay(isOn ? 1.0f : off); // set overlay factor
}

private:
    bool enabled = false;
    uint32_t period = 1000;
    uint32_t duty = 50;
    float off = 0.0f; // 0.0 = volledig uit, 1.0 = geen effect
    uint32_t phaseStart = 0;
};