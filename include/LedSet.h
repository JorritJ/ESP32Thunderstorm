#pragma once
#include <Arduino.h>
#include "LedPwm.h"

class LedSet {
public:
  
  // channels = array van pointers of referenties is ook prima; hier nemen we pointer naar array
  LedSet(LedPwmChannel** channels, int count, const float* weights /*size=count*/)
    : chans(channels), n(count), w(weights) {}
  
  void setOverlay(float f) {             // 0..1
    if (f < 0) f = 0; if (f > 1) f = 1;
    overlayFactor = f;
  }

  void setAllScaled(uint16_t duty) {
    for (int i = 0; i < n; ++i) {
      uint32_t d = duty;
      if (w) d = (uint32_t)(w[i] * d);
      d = (uint32_t)(overlayFactor * d);  // ← overlay toegepast
      if (d > 65535u) d = 65535u;
      chans[i]->setDuty((uint16_t)d);
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
  float overlayFactor = 1.0f; // 1.0 = geen effect, 0.0 = volledig uit
};