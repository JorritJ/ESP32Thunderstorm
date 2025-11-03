// --- file: Button.cpp
#include "Button.h"

Button::Button(int p, bool aLow) : pin(p), activeLow(aLow), state(false), lastState(false), pressedEvent(false), lastChangeMs(0) {}

void Button::begin()
{
    pinMode(pin, INPUT_PULLUP);
    state = readRaw();
    lastState = state;
}

bool Button::readRaw() const
{
    int v = digitalRead(pin);
    return activeLow ? (v == LOW) : (v == HIGH);
}

void Button::update(uint32_t now)
{
    bool raw = readRaw();
    if (raw != lastState && (now - lastChangeMs) >= BTN_DEBOUNCE_MS)
    {
        lastState = raw;
        lastChangeMs = now;
        state = raw;
        if (state)
            pressedEvent = true;
    }
}

bool Button::consumePressed()
{
    if (pressedEvent)
    {
        pressedEvent = false;
        return true;
    }
    return false;
}