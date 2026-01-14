#pragma once
#include <ESPAsyncWebServer.h>

AsyncWebServer* setupWeb();
void broadcastSensorData();
