#pragma once
#include <ESPAsyncWebServer.h>
#include "context.h"

class ApiController {
public:
  ApiController(GlobalContext *context);
private:
  AsyncWebServer *server;
  GlobalContext *context;
};
