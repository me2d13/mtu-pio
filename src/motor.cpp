#include "motor.h"
#include "config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "Logger.h"
#include "context.h"
#include <ReactESP.h>


HardwareSerial motorSerial(1);

#define R_SENSE 0.11f

TMC2208Stepper driver(&motorSerial, R_SENSE);


// Constructor implementation
Motor::Motor(int motorIndex, int step, int dir, int rmsCurrent, int steps) 
    : index(motorIndex), 
      stepPin(step), 
      dirPin(dir), 
      rmsCurrent(rmsCurrent), 
      microsteps(steps) {}

// Getter implementations
int Motor::getIndex() const { return index; }
int Motor::getRmsCurrent() const { return rmsCurrent; }
int Motor::getMicrosteps() const { return microsteps; }

// Setter implementations
void Motor::setRmsCurrent(int rms) { rmsCurrent = rms; }
void Motor::setMicrosteps(int steps) { microsteps = steps; }

void Motor::init() {
    selectMotor(index);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    driver.begin();
    driver.toff(5);                 // Enables driver in software
    driver.rms_current(rmsCurrent); // Set motor RMS current
    driver.microsteps(microsteps);  // Set microsteps to 1/8th

    driver.pwm_autoscale(true);     // Needed for stealthChop

    uint8_t  version    = driver.version();
    uint16_t microsteps = driver.microsteps();
    std::stringstream ss;
    ss << "Motor " << index << " performing init - Driver version: " << static_cast<int>(version) 
        << ", Microsteps: " << microsteps;
    logger.log(ss.str());
    driver.VACTUAL(0);
}

void Motor::stepCallback() {
    if (stepsToMake == 0) {
        std::stringstream ss;
        ss << "Motor " << index << " finished step turn";
        logger.log(ss.str());
        steppingEvent->remove(&ctx()->eventLoop);
        steppingEvent = NULL;
    } else {
        digitalWrite(stepPin, HIGH);
        digitalWrite(stepPin, LOW);
        if (stepsToMake > 0) {
            stepsToMake--;
        } else {
            stepsToMake++;
        }
    }
}

void Motor::turnBySpeed(int speed) {
    std::stringstream ss;
    ss << "Motor " << index << " running at speed " << static_cast<int>(speed);
    logger.log(ss.str());
    driver.VACTUAL(microsteps == 0 ? speed : speed * microsteps);
}

void Motor::makeSteps(int angle, int rpm) {
    int microstepsOrOne = (microsteps == 0) ? 1 : microsteps;
    stepsToMake = (200 * angle * microstepsOrOne) / 360;
    int pulsesByRevolution = 200 * microstepsOrOne;
    float stepsPerSecond = rpm * pulsesByRevolution / 60.0;
    stepDelay = round(1000.0 / stepsPerSecond);
    std::stringstream ss;
    ss << "MakeSteps: motor " << index << " - for " << static_cast<int>(stepsToMake) 
        << " steps to make calculated delay in ms between pulses: " << stepDelay;
    logger.log(ss.str());
    if (steppingEvent != NULL) {
        std::stringstream ss;
        ss << "Motor " << index << " active stepping event found, removing...";
        logger.log(ss.str());
        steppingEvent->remove(&ctx()->eventLoop);
    }
    digitalWrite(dirPin, (stepsToMake > 0) ? HIGH : LOW);
    steppingEvent = ctx()->eventLoop.onRepeat(stepDelay, [&]() { stepCallback(); });
}

void setupMotors() {
    pinMode(PIN_MOTOR_ADDR_0, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_1, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_2, OUTPUT);
    motorSerial.begin(115200, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
}

void selectMotor(uint8_t addr) {
    digitalWrite(PIN_MOTOR_ADDR_0, addr & 1);
    digitalWrite(PIN_MOTOR_ADDR_1, (addr & 2) >> 1);
    digitalWrite(PIN_MOTOR_ADDR_2, (addr & 4) >> 2);
}