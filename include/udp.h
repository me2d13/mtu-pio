#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include "config.h"
#include "state.h"
#include <ArduinoJson.h>

class SimUdpInterface
{
private:
    Task udpCheckTask;
    char errorMessage[100];
    int errorsLogCount = 0;
    void parsePacket(char *buffer, int len);
    void parseXplaneData(JsonObject data);

    static float convertMsfsTrim(float simValue);

    void parseMsfsData(JsonObject data);
    void loopUdp();
    void logError();
    void commitChange(sim_data backup);
public:
    void setup();
    // A payload in either simulator format (X-Plane datarefs or MSFS variable names).
    bool applyPayload(JsonObject data);
    // Values posted from the web mock using friendly keys; ends up in exactly the same
    // sim_data struct and the same driver callbacks as a UDP packet.
    String applyMock(JsonObject data);
};
