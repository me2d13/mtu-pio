#include "motor.h"
#include "config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "Logger.h"
#include "context.h"
#include <TaskSchedulerDeclarations.h>

int calculateMoveSpeed(long currentPosition, long targetPosition);

// Constructor implementation
Motor::Motor(int motorIndex, int uartChannel, int step, int dir) 
    : index(motorIndex), 
      uartChannel(uartChannel),
      stepPin(step), 
      dirPin(dir) {
        // Initialize the stepping task
        steppingTask = new Task(0, TASK_ONCE, [&]() { stepCallback(); }, &ctx()->taskScheduler, false);
        movingTask = new Task(100, TASK_FOREVER, [&]() { moveCallback(); }, &ctx()->taskScheduler, false);
        settings = &ctx()->state.persisted.motorSettings[motorIndex];
      }

// Getter implementations
int Motor::getIndex() const { return index; }

void Motor::init(TMC2208Stepper *driver) {
    this->driver = driver;
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    char message[70];
    snprintf(message, sizeof(message), "Motor %d initialized with step pin: %d, dir pin: %d, uart addr %d", index, stepPin, dirPin, uartChannel);
    logger.log(message);
    reInitDriver();
}

void Motor::reInitDriver() {
    disable();
    ctx()->motorsController.selectMotorUart(uartChannel);
    driver->begin();
    driver->toff(5);                 // Enables driver in software
    driver->rms_current(settings->runCurrent); // Set motor RMS current
    driver->microsteps(settings->microSteps);  // Set microsteps to 1/8th

    driver->pwm_autoscale(true);     // Needed for stealthChop

    uint8_t  version    = driver->version();
    uint16_t microsteps = driver->microsteps();
    if (version) {
        std::stringstream ss;
        ss << "Motor " << index << " performing init - Driver version: " << static_cast<int>(version) 
            << ", Microsteps: " << microsteps;
        logger.log(ss.str());
    } else {
        logger.log("WARN Motor " + std::to_string(index) + " driver communication problem, no response received.");
    }
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
        disable();
        ctx()->simDataDriver.motorStoppedAtPosition(index);
    } else {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(stepPin, LOW);
        if (stepsToMake > 0) {
            stepsToMake--;
        } else {
            stepsToMake++;
        }
    }
}

void Motor::moveCallback() {
    if (axisIndex == -1) {
        return;
    }
    long currentPosition = ctx()->state.transient.getCalibratedAxisValue(axisIndex, &ctx()->state.persisted.axisSettings[axisIndex]);
    auto speed = calculateMoveSpeed(currentPosition, targetPosition);
    if (speed == 0) {
        std::stringstream ss;
        ss << "Motor " << index << " reached target position " << static_cast<int>(targetPosition);
        logger.log(ss.str());
        ctx()->motorsController.selectMotorUart(uartChannel);
        driver->VACTUAL(0);
        movingSpeed = 0;
        movingTask->disable();
        disable();
        ctx()->simDataDriver.motorStoppedAtPosition(index);
    } else {
        if (sensorReversed) {
            speed = -speed;
        }
        if (movingSpeed != speed) {
            // just decelerate as running closer
            turnBySpeed(speed);
            movingSpeed = speed;
        }
    }
}

void Motor::turnBySpeed(int speed) {
    ctx()->motorsController.selectMotorUart(uartChannel);
    std::stringstream ss;
    ss << "Motor " << index << " running at speed " << static_cast<int>(speed);
    logger.log(ss.str());
    driver->VACTUAL(settings->microSteps == 0 ? speed : speed * settings->microSteps);
    if (speed == 0) {
        // also disable moving task
        if (movingTask->isEnabled()) {
            movingTask->disable();
        }
    }
}

void Motor::makeSteps(float angle, int rpm) {
    int microstepsOrOne = (settings->microSteps == 0) ? 1 : settings->microSteps;
    stepsToMake = round((200.0f * angle * microstepsOrOne) / 360.0f);
    int pulsesByRevolution = 200 * microstepsOrOne;
    float stepsPerSecond = (( float) rpm * pulsesByRevolution) / 60.0f;
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
    enable();
}

// similar to makeSteps but adding angle to target steps
// if motor is not moving, behaves exactly as makeSteps
// if motor is moving, it will add steps to the current position and rpm is ignored
void Motor::addSteps(float angle, int rpm) {
    if (!steppingTask->isEnabled()) {
        makeSteps(angle, rpm);
        return;
    }
    int microstepsOrOne = (settings->microSteps == 0) ? 1 : settings->microSteps;
    int deltaStepsToMake = round((200.0f * angle * microstepsOrOne) / 360.0f);
    stepsToMake += deltaStepsToMake;
    digitalWrite(dirPin, (stepsToMake > 0) ? HIGH : LOW);
}

void Motor::enable() {
    // GPB0 is index 8
    logger.print("Enabling motor ");
    logger.println(index);
    ctx()->pins.setPin(uartChannel + 8, LOW);
    // W21-15, R21-00, W21-13-00
}

void Motor::disable() {
    logger.print("Disabling motor ");
    logger.println(index);
    ctx()->pins.setPin(uartChannel + 8, HIGH);
}

int Motor::calculateMoveSpeed(long currentPosition, long targetPosition) {
    long distance = abs(targetPosition - currentPosition);
    int speed = 0;
    if (distance < AXIS_MAX_CALIBRATED_VALUE / 200) { // 0.5% of the range
        return 0;
    } else if (distance < AXIS_MAX_CALIBRATED_VALUE / 100) { // 1% of the range
        speed = round(2.0f * settings->speedMultiplier);
    } else if (distance < AXIS_MAX_CALIBRATED_VALUE / 20) { // 5% of the range
        speed = round(10.0f * settings->speedMultiplier);
    } else if (distance < AXIS_MAX_CALIBRATED_VALUE / 10) { // 10% of the range
        speed = round(30.0f * settings->speedMultiplier);;
    } else {
        speed = round(50.0f * settings->speedMultiplier);;
    }
    return (currentPosition < targetPosition) ? speed : -speed;
}

void Motor::moveToPosition(long position) {
    if (axisIndex == -1) {
        std::stringstream ss;
        ss << "Motor " << index << " has no axis assigned, cannot move to position " << static_cast<int>(position);
        logger.log(ss.str());
        return;
    }
    long currentPosition = ctx()->state.transient.getCalibratedAxisValue(axisIndex, &ctx()->state.persisted.axisSettings[axisIndex]);
    auto speed = calculateMoveSpeed(currentPosition, position);
    if (speed == 0) {
        ctx()->simDataDriver.motorStoppedAtPosition(index);
        std::stringstream ss;
        ss << "Motor " << index << " already at target position " << static_cast<int>(position);
        logger.log(ss.str());
        return;
    }
    if (sensorReversed) {
        speed = -speed;
    }
    movingSpeed = speed;
    enable();
    turnBySpeed(speed);
    targetPosition = position;
    // start the task to watch position
    if (!movingTask->isEnabled()) {
        movingTask->enable();
    }
    std::stringstream ss;
    ss << "Motor " << index << " currently at position " << static_cast<int>(currentPosition) << " started moving to position " 
        << static_cast<int>(position) << " at speed " << static_cast<int>(speed);
    logger.log(ss.str());
}

void Motor::stopMotor() {
    if (movingTask->isEnabled()) {
        movingTask->disable();
    }
    if (steppingTask->isEnabled()) {
        steppingTask->disable();
    }
    driver->VACTUAL(0);
    disable();
    std::stringstream ss;
    ss << "Motor " << index << " stopped";
    logger.log(ss.str());
}