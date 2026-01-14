#pragma once
#include <ESPAsyncWebServer.h>
#include "context.h"
#include <TaskSchedulerDeclarations.h>

class ApiController {
public:
  ApiController(GlobalContext *context);
  void setup();
private:
  AsyncWebServer *server;
  GlobalContext *context;
  Task sensorBroadcastTask;
};
