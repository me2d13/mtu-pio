#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include "config.h"

class XplaneInterface
{
private:
    Task udpCheckTask;
    char errorMessage[100];
    int errorsLogCount = 0;
    void parsePacket(char *buffer, int len);
    void loopUdp();
    void logError();
public:
    void setup();
};
