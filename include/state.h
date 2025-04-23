#pragma once
#include <Arduino.h>
#include "config.h"

struct axis_settings
{
    // if minValue > maxValue there are 2 posible scenarios:
    // 1. the axis is reversed and the value is in the range of minValue and maxValue
    // 2. the axis is not reversed sensor raw angle overflows from 4095 to 0
    // these 2 scenarios are identified by the isReversed flag
    int minValue;
    int maxValue;
    char name[6];
    bool isReversed;
};


class PersistedState
{
private:
public:
    axis_settings axisSettings[NUMBER_OF_AXIS] = {
        {0, 4096, "SB", false},
        {0, 4096, "THR1", false},
        {2505, 846, "THR2", false},
        {0, 4096, "FLAPS", false},
        {0, 4096, "TRIM", false}
    };
String reportState();
};

class TransientState
{
private:
    int axisValues[NUMBER_OF_AXIS];
    bool twelveVPresent = false;
    int i2cChannelSwitchFailures = 0;
    int axisReadFailures[NUMBER_OF_AXIS];
public:
    TransientState() {
        for (int i = 0; i < NUMBER_OF_AXIS; i++) {
            axisValues[i] = 0;
            axisReadFailures[i] = 0;
        }
    }
    int getAxisValue(int index);
    int getCalibratedAxisValue(int index, axis_settings *settings);
    void setAxisValue(int index, int value);
    void set12Vpresent(bool present) { this->twelveVPresent = present; }
    bool is12Vpresent() { return this->twelveVPresent; }
    int incrementI2cChannelSwitchFailures() { return ++this->i2cChannelSwitchFailures; }
    int getI2cChannelSwitchFailures() { return this->i2cChannelSwitchFailures; }
    int incrementAxisReadFailures(int index) { return ++this->axisReadFailures[index]; }
    int getAxisReadFailures(int index) { return this->axisReadFailures[index]; }
    String reportState();
};

class State
{
private:
public:
    PersistedState persisted;
    TransientState transient;
};

