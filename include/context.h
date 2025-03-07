#ifndef CONTEXT_H
#define CONTEXT_H

#include <Arduino.h>
#include <Wire.h>
#include <ReactESP.h>
#include <ESPAsyncWebServer.h>
#include "i2c.h"

class GlobalContext {
public:
  explicit GlobalContext();
  reactesp::EventLoop eventLoop;
  
  void setup();
  AsyncWebServer* getServer();
  I2cController* i2c() { return i2cController; };
private:
  AsyncWebServer* server;
  I2cController* i2cController;
};

GlobalContext* ctx();

#endif //CONTEXT_H