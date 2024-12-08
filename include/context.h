#ifndef CONTEXT_H
#define CONTEXT_H

#include <Arduino.h>
#include <Wire.h>
#include <ReactESP.h>

class GlobalContext {
public:
  explicit GlobalContext();
  reactesp::EventLoop eventLoop;
  TwoWire* i2C1; // all peripherals (LCD, 2017)
  TwoWire* i2C2; // sensors
  void setup();
};

GlobalContext* ctx();

#endif //CONTEXT_H