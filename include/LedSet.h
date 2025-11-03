#pragma once
#include <Arduino.h>
#include "LedPwm.h"

class LedSet {
public:
  // channels = array van pointers of referenties is ook prima; hier nemen we pointer naar array
  LedSet(LedPwmChannel** channels, int count, const float* weights /*size=count*/)
    : chans(channels), n(count), w(weights) {}

  void setAllScaled(uint16_t duty) {
    // duty wordt per kanaal gewogen met w[i] (0..1)
    for (int i = 0; i < n; ++i) {
      uint16_t d = (uint16_t)(w ? (w[i] * duty) : duty);
      chans[i]->setDuty(d);
    }
  }
  void setAll(uint16_t duty) { // geen weight
    for (int i = 0; i < n; ++i) chans[i]->setDuty(duty);
  }
  uint16_t maxDuty() const {
    // aangenomen: alle kanalen zelfde resolutie — pak de 1e
    return n > 0 ? chans[0]->maxDuty() : 4095;
  }
  int size() const { return n; }

private:
  LedPwmChannel** chans;
  int n;
  const float* w; // per-kanaal gewicht (0..1), kan null zijn
};