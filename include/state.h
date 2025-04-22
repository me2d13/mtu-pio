#pragma once
#include <Arduino.h>
#include "config.h"

class PersistedState
{
private:
public:
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

