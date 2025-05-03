#ifndef CONTEXT_H
#define CONTEXT_H

#include <Arduino.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include "i2c.h"
#include <TaskSchedulerDeclarations.h>
#include "pins.h"
#include "motors.h"
#include "axis.h"
#include "state.h"
#include "udp.h"
#include "Screen.h"

class GlobalContext {
public:
  explicit GlobalContext();
  Scheduler taskScheduler;
  
  void setup();
  AsyncWebServer* getServer();
  I2cController* i2c() { return i2cController; };
  MultiplexedPins pins;
  MotorsController motorsController;
  AxesController axesController;
  State state;
  XplaneInterface xplaneInterface;
  ScreenController screenController;
  void debugCall(int index);
private:
  AsyncWebServer* server;
  I2cController* i2cController;
};

GlobalContext* ctx();

#endif //CONTEXT_H