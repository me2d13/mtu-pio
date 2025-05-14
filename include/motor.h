#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "TaskSchedulerDeclarations.h"
#include "state.h"


class Motor {
private:
/* index and uart channel had to be separated because of wrong HW design when HW addresses 4 and 5 have STEP & DIR pin
  connected to reserved pins for ESP32 so at the end can't be used.
  As trim indicators are the only steppers which require controll by step pin (others can be controlled by UART) 
  they were switched with throttles to position 0 and 1 */
    int index; // index in motors array, TH1 = 0, TH2 = 1, SB = 2, TRIM = 3, TRIM_IND_1 = 4, TRIM_IND_2 = 5
    int uartChannel; // HW address, TH1 = 4, TH2 = 5, SB = 2, TRIM = 3, TRIM_IND_1 = 0, TRIM_IND_2 = 1
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
    int calculateMoveSpeed(long currentPosition, long targetPosition);
public:
    // Constructor with default values for rmsCurrent and microsteps
    Motor(int motorIndex, int uartChannel, int step, int dir);
    void addSensor(int axisIndex, bool motorSensorReversed) {
        this->axisIndex = axisIndex;
        this->sensorReversed = motorSensorReversed;
    }

    // Getters
    int getIndex() const;

    void init(TMC2208Stepper *driver);
    void reInitDriver();
    void stepCallback();
    void moveCallback();
    void turnBySpeed(int speed);
    void makeSteps(float angle, int rpm);
    void addSteps(float angle, int rpm);
    void enable();
    void disable();
    void debugCall();
    void moveToPosition(long position);
    void stopMotor();
};

#endif // MOTOR_H
