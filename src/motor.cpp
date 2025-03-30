#include "motor.h"
#include "config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "Logger.h"
#include "context.h"
#include <TaskSchedulerDeclarations.h>



// Constructor implementation
Motor::Motor(int motorIndex, int step, int dir, int rmsCurrent, int steps) 
    : index(motorIndex), 
      stepPin(step), 
      dirPin(dir), 
      rmsCurrent(rmsCurrent), 
      microsteps(steps) {
    // Initialize the stepping task
    steppingTask = new Task(0, TASK_ONCE, [&]() { stepCallback(); }, &ctx()->taskScheduler, false);
      }

// Getter implementations
int Motor::getIndex() const { return index; }
int Motor::getRmsCurrent() const { return rmsCurrent; }
int Motor::getMicrosteps() const { return microsteps; }

// Setter implementations
void Motor::setRmsCurrent(int rms) { rmsCurrent = rms; }
void Motor::setMicrosteps(int steps) { microsteps = steps; }

void Motor::init(TMC2208Stepper *driver) {
    disable();
    this->driver = driver;
    ctx()->motorsController.selectMotorUart(index);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    driver->begin();
    driver->toff(5);                 // Enables driver in software
    driver->rms_current(rmsCurrent); // Set motor RMS current
    driver->microsteps(microsteps);  // Set microsteps to 1/8th

    driver->pwm_autoscale(true);     // Needed for stealthChop

    uint8_t  version    = driver->version();
    uint16_t microsteps = driver->microsteps();
    std::stringstream ss;
    ss << "Motor " << index << " performing init - Driver version: " << static_cast<int>(version) 
        << ", Microsteps: " << microsteps;
    logger.log(ss.str());
    driver->VACTUAL(0);
}

void Motor::debugCall() {
    uint8_t  version    = driver->version();
    uint16_t microsteps = driver->microsteps();
    std::stringstream ss;
    ss << "Motor " << index << " performing debug call - Driver version: " << static_cast<int>(version) 
        << ", Microsteps: " << microsteps;
    logger.log(ss.str());
}


void Motor::stepCallback() {
    if (stepsToMake == 0) {
        std::stringstream ss;
        ss << "Motor " << index << " finished step turn";
        logger.log(ss.str());
        steppingTask->disable();
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
    ctx()->motorsController.selectMotorUart(index);
    std::stringstream ss;
    ss << "Motor " << index << " running at speed " << static_cast<int>(speed);
    logger.log(ss.str());
    driver->VACTUAL(microsteps == 0 ? speed : speed * microsteps);
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
    if (steppingTask->isEnabled()) {
        std::stringstream ss;
        ss << "Motor " << index << " active stepping event found, removing...";
        logger.log(ss.str());
        steppingTask->disable();
    }
    digitalWrite(dirPin, (stepsToMake > 0) ? HIGH : LOW);
    steppingTask->setInterval(stepDelay);
    steppingTask->setIterations(TASK_FOREVER);
    steppingTask->enable();
}

void Motor::enable() {
    // GPB0 is index 8
    logger.print("Enabling motor ");
    logger.println(index);
    ctx()->pins.setPin(index + 8, LOW);
    // W21-15, R21-00, W21-13-00
}

void Motor::disable() {
    logger.print("Disabling motor ");
    logger.println(index);
    ctx()->pins.setPin(index + 8, HIGH);
}