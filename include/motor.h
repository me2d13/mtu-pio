#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "TaskSchedulerDeclarations.h"
#include "state.h"


class Motor {
private:
    int index;
    int stepPin;
    int dirPin; 
    int stepDelay = 0;
    int stepsToMake = 0;
    int axisIndex = -1;
    bool sensorReversed = false;
    long targetPosition = 0;
    int movingSpeed = 0;
    Task *steppingTask;
    Task *movingTask;
    TMC2208Stepper *driver;
    motor_settings *settings;

public:
    // Constructor with default values for rmsCurrent and microsteps
    Motor(int motorIndex, int step, int dir);
    void addSensor(int axisIndex, bool motorSensorReversed) {
        this->axisIndex = axisIndex;
        this->sensorReversed = sensorReversed;
    }

    // Getters
    int getIndex() const;

    void init(TMC2208Stepper *driver);
    void stepCallback();
    void moveCallback();
    void turnBySpeed(int speed);
    void makeSteps(int numberOfSteps, int rpm);
    void enable();
    void disable();
    void debugCall();
    void moveToPosition(long position);
};

#endif // MOTOR_H
