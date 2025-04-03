#pragma once
#include <Wire.h>
#include "TaskSchedulerDeclarations.h"
#include "config.h"
#include <AS5600.h>

class AxesController
{
private:
    Task axesCheckTask;
    int axisValues[NUMBER_OF_AXIS];
    void readAxisData();
    AS5600* sensor;
    uint32_t measureIntervalStart = 0;
    uint16_t measuredSamples = 0;
    uint32_t measureTimeSpent = 0;
public:
    void setup();
    int getAxisValue(int index);
};
