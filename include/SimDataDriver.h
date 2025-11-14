#pragma once
#include <Arduino.h>
#include "state.h"
#include "TaskSchedulerDeclarations.h"


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
    void throttleChanged(float oldValue, float newValue, bool autoThrottle);
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
    // last value which triggered motor move
    float lastActionValue = -1.0f;
    Task trimWheelStopTask;
    Task calibrationTask;
    int calibrationPhase = 0;
    unsigned long calibrationStartTime;
    bool isEndStop1On();
    bool isEndStop2On();
    void calibrationTaskCallback();
public:
    TrimDriver(int motorIndexInd1, int motorIndexInd2, int motorIndexTrimWheel, const char *name) : 
        motorIndexInd1(motorIndexInd1), motorIndexInd2(motorIndexInd2), motorIndexTrimWheel(motorIndexTrimWheel) {
            state.name = name;
            state.requestedValue = 0.0f;
            state.requestedPosition = 0;
            state.currentValue = 0.0f;
            state.currentPosition = -1;
        }
    void setup();
    void trimChanged(float oldValue, float newValue);
    void motorStoppedAtPosition();
    int getMotorIndexInd1() { return motorIndexInd1; }
    int getMotorIndexInd2() { return motorIndexInd2; }
    int getMotorIndexTrimWheel() { return motorIndexTrimWheel; }
    int getCalibrationPhase() { return calibrationPhase; }
    driver_state *getState();
    void calibrate();
};
    
class SpeedBrakeDriver
{
private:
    int axisIndex;
    int motorIndex;
    driver_state state;
public:
    SpeedBrakeDriver(int axisIndex, int motorIndex, const char *name) : axisIndex(axisIndex), motorIndex(motorIndex) {
        state.name = name;
        state.requestedValue = 0.0f;
        state.requestedPosition = 0;
        state.currentValue = 0.0f;
        state.currentPosition = 0;
        state.controlMode = FREE;
    }
    void speedBrakeChanged(float oldValue, float newValue);
    bool canSendJoyValue();
    void motorStoppedAtPosition();
    int getAxisIndex() { return axisIndex; }
    int getMotorIndex() { return motorIndex; }
    driver_state *getState();
};

class SimDataDriver
{
private:
    ThrottleDriver *throttle1;
    ThrottleDriver *throttle2;
public:
    TrimDriver *trim;
    SpeedBrakeDriver *speedBrake;
    SimDataDriver() {
        throttle1 = new ThrottleDriver(AXIS_INDEX_THROTTLE_1, MOTOR_INDEX_THROTTLE_1, "Throttle 1 ");
        throttle2 = new ThrottleDriver(AXIS_INDEX_THROTTLE_2, MOTOR_INDEX_THROTTLE_2, "Throttle 2 ");
        trim = new TrimDriver(MOTOR_INDEX_TRIM_IND_1, MOTOR_INDEX_TRIM_IND_2, MOTOR_INDEX_TRIM, "Trim       ");
        speedBrake = new SpeedBrakeDriver(AXIS_INDEX_SPEED_BRAKE, MOTOR_INDEX_SPEED_BRAKE, "Speed brake");
    }
    ~SimDataDriver() {
        delete throttle1;
        delete throttle2;
        delete trim;
        delete speedBrake;
    }
    void simDataChanged(sim_data &oldSimData, sim_data &newSimData);
    bool canSendJoyValue(int axisIndex);
    void motorStoppedAtPosition(int motorIndex);
    driver_state *getState(int index);
    void setup();
    void calibrate();
};
