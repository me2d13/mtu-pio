#pragma once
#include <Arduino.h>
#include "state.h"


enum AxisControlMode {
    FREE,
    CHASE,
};

struct driver_state {
    const char *name;
    float requestedValue;
    int requestedPosition;
    float currentValue;
    int currentPosition;
    AxisControlMode controlMode;
};

class ThrottleDriver
{
private:
    int axisIndex;
    int motorIndex;
    driver_state state;
public:
    ThrottleDriver(int axisIndex, int motorIndex, const char *name) : axisIndex(axisIndex), motorIndex(motorIndex) {
        state.name = name;
        state.requestedValue = 0.0f;
        state.requestedPosition = 0;
        state.currentValue = 0.0f;
        state.currentPosition = 0;
        state.controlMode = FREE;
    }
    void throttleChanged(float oldValue, float newValue);
    bool canSendJoyValue();
    void motorStoppedAtPosition();
    int getAxisIndex() { return axisIndex; }
    int getMotorIndex() { return motorIndex; }
    driver_state *getState();
};

class TrimDriver
{
private:
    int motorIndexInd1;
    int motorIndexInd2;
    int motorIndexTrimWheel;
    driver_state state;
public:
    TrimDriver(int motorIndexInd1, int motorIndexInd2, int motorIndexTrimWheel, const char *name) : 
        motorIndexInd1(motorIndexInd1), motorIndexInd2(motorIndexInd2), motorIndexTrimWheel(motorIndexTrimWheel) {
            state.name = name;
            state.requestedValue = 0.0f;
            state.requestedPosition = 0;
            state.currentValue = 0.0f;
            state.currentPosition = -1;
        }
    void trimChanged(float oldValue, float newValue);
    void motorStoppedAtPosition();
    int getMotorIndexInd1() { return motorIndexInd1; }
    int getMotorIndexInd2() { return motorIndexInd2; }
    int getMotorIndexTrimWheel() { return motorIndexTrimWheel; }
    driver_state *getState() { return &state; }
};
    

class SimDataDriver
{
private:
    ThrottleDriver *throttle1;
    ThrottleDriver *throttle2;
    TrimDriver *trim;
public:
    SimDataDriver() {
        throttle1 = new ThrottleDriver(AXIS_INDEX_THROTTLE_1, MOTOR_INDEX_THROTTLE_1, "Throttle 1 ");
        throttle2 = new ThrottleDriver(AXIS_INDEX_THROTTLE_2, MOTOR_INDEX_THROTTLE_2, "Throttle 2 ");
        trim = new TrimDriver(MOTOR_INDEX_TRIM_IND_1, MOTOR_INDEX_TRIM_IND_2, MOTOR_INDEX_TRIM, "Trim       ");
    }
    ~SimDataDriver() {
        delete throttle1;
        delete throttle2;
        delete trim;
    }
    void simDataChanged(xpl_data &oldXplData, xpl_data &newXplData);
    bool canSendJoyValue(int axisIndex);
    void motorStoppedAtPosition(int motorIndex);
    driver_state *getState(int index);
};
