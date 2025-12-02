#pragma once
#include <Arduino.h>
#include "Config.h"

//klasse voor aansturing van lantaarnpalen: zet een pin hoog/laag

class LanternController {
public:
  void begin() {
    pinMode(Config::LANTERN_PIN, OUTPUT);
    off();
  }
  void on()  { digitalWrite(Config::LANTERN_PIN, Config::LANTERN_ACTIVE_HIGH ? HIGH : LOW);  _isOn = true;  }
  void off() { digitalWrite(Config::LANTERN_PIN, Config::LANTERN_ACTIVE_HIGH ? LOW  : HIGH); _isOn = false; }
  void set(bool enable) { enable ? on() : off(); }
  bool isOn() const { return _isOn; }

private:
  bool _isOn = false;
};
