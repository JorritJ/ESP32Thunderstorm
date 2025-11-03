// --- file: Button.h
#pragma once
#include <Arduino.h>

constexpr uint32_t BTN_DEBOUNCE_MS = 25; // debounce delay

struct Button
{
    int pin;
    bool activeLow;
    bool state;
    bool lastState;
    bool pressedEvent;
    uint32_t lastChangeMs;

    Button(int p, bool aLow = true);
    void begin();
    bool readRaw() const;
    void update(uint32_t now);
    bool consumePressed();
};