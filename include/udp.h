#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include "config.h"
#include <ArduinoJson.h>

class SimUdpInterface
{
private:
    Task udpCheckTask;
    char errorMessage[100];
    int errorsLogCount = 0;
    void parsePacket(char *buffer, int len);
    void parseXplaneData(JsonDocument &doc);

    static float convertMsfsTrim(float simValue);

    void parseMsfsData(JsonDocument &doc);
    void loopUdp();
    void logError();
public:
    void setup();
};
