#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include <ReactESP.h>

class Motor {
private:
    int index;
    int rmsCurrent;
    int microsteps;
    int stepPin;
    int dirPin; 
    int stepDelay = 0;
    int stepsToMake = 0;
    reactesp::RepeatEvent* steppingEvent = NULL;

public:
    // Constructor with default values for rmsCurrent and microsteps
    Motor(int motorIndex, int step, int dir, int rmsCurrent = 100, int steps = 4);

    // Getters
    int getIndex() const;
    int getRmsCurrent() const;
    int getMicrosteps() const;

    // Setters
    void setRmsCurrent(int rms);
    void setMicrosteps(int steps);

    void init();
    void stepCallback();
    void turnBySpeed(int speed);
    void makeSteps(int numberOfSteps, int rpm);
};

// Function declarations
void setupMotors();
void selectMotor(uint8_t addr);

extern HardwareSerial motorSerial;
extern TMC2208Stepper driver;

#endif // MOTOR_H
