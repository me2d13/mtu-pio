#include <Arduino.h>
#include "SimDataDriver.h"
#include "context.h"
#include "state.h"
#include "config.h"
#include "Logger.h"

#define NOT_LOG_DECISIONS 
// 5 degrees of trim are 22 degrees of stepper
#define STEPPER_ANGLE_PER_TRIM_ANGLE (22.0f/5.0f)
#define TRIM_IND_VELOCITY 3

void ThrottleDriver::throttleChanged(float oldValue, float newValue) {
    int requestedPosition = newValue * (float) AXIS_MAX_CALIBRATED_VALUE;
    int currentPosition = ctx()->state.transient.getCalibratedAxisValue(axisIndex, &ctx()->state.persisted.axisSettings[axisIndex]);
    int delta = abs(currentPosition - requestedPosition);
    #ifdef LOG_DECISIONS
        logger.print(state.name);
        logger.print(" changed from ");
        logger.print(oldValue);
        logger.print(" to ");
        logger.print(newValue);
        logger.print(" delta: ");
        logger.println(delta);
    #endif
    int timeFromLastManualUpdate = millis() - ctx()->state.transient.getLastAxisJoyUpdate(axisIndex);
    // we start only on change of 0.5%
    if (delta > AXIS_MAX_CALIBRATED_VALUE / 200 && timeFromLastManualUpdate > MOTORIZED_UPDATE_IGNORE_INTERVAL) {
        state.requestedValue = newValue;
        state.requestedPosition = requestedPosition;
        logger.print("Because of update from simulator, moving throttle to ");
        logger.println(state.requestedPosition);
        ctx()->motorsController.getMotor(motorIndex)->moveToPosition(state.requestedPosition);
        state.controlMode = CHASE; // joystick updates won't be sent until motor stops to not receive updates from simulator
    }
}

bool ThrottleDriver::canSendJoyValue() {
    return (state.controlMode == FREE);
}

void ThrottleDriver::motorStoppedAtPosition() {
    if (state.controlMode == CHASE) {
        state.controlMode = FREE; // motor stopped, we can send joystick updates again
    }
}

driver_state *ThrottleDriver::getState() {
    state.currentValue = ctx()->state.transient.getAxisValue(axisIndex);
    state.currentPosition = ctx()->state.transient.getCalibratedAxisValue(axisIndex, &ctx()->state.persisted.axisSettings[axisIndex]);
    return &state;
}

void TrimDriver::trimChanged(float oldValue, float newValue) {
    if (oldValue < 0) {
        // ignore, this is initial value, let's wait for real delta
        return;
    }
    float trimAngleChange = newValue - oldValue;
    if (abs(trimAngleChange) > 0.001f) {
        state.controlMode = CHASE;
        float stepperAngleChange = trimAngleChange * STEPPER_ANGLE_PER_TRIM_ANGLE;
        #ifdef LOG_DECISIONS
        char message[200];
        sprintf(message, "Sim trim changed by %f, ordering stepper angle change by %f.", trimAngleChange, stepperAngleChange);
        logger.log(message);
        #endif
        ctx()->motorsController.getMotor(motorIndexInd1)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        ctx()->motorsController.getMotor(motorIndexInd2)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        auto wheelVelocity = trimAngleChange < 0 ? ctx()->state.persisted.trimWheelVelocity : -ctx()->state.persisted.trimWheelVelocity;
        ctx()->motorsController.getMotor(motorIndexTrimWheel)->enable();
        ctx()->motorsController.getMotor(motorIndexTrimWheel)->turnBySpeed(wheelVelocity);
    }
}

void TrimDriver::motorStoppedAtPosition() {
    state.controlMode = FREE;
    ctx()->motorsController.getMotor(motorIndexTrimWheel)->turnBySpeed(0);
    ctx()->motorsController.getMotor(motorIndexTrimWheel)->disable();
}

void SimDataDriver::simDataChanged(xpl_data &oldXplData, xpl_data &newXplData) {
    // This function is called when the sim data changes
    // It can be used to update the state or perform any other necessary actions
    // based on the data received from X-Plane.
    // For example, you can check if the parking brake state has changed and update the indicator accordingly.
    if (oldXplData.parkingBrake != newXplData.parkingBrake) {
        ctx()->pins.setParkingBrakeIndicator(newXplData.parkingBrake);
    }
    throttle1->throttleChanged(oldXplData.throttle1, newXplData.throttle1);
    throttle2->throttleChanged(oldXplData.throttle2, newXplData.throttle2);
    trim->trimChanged(oldXplData.trim, newXplData.trim);
}

bool SimDataDriver::canSendJoyValue(int axisIndex) {
    // sometimes when the motor is moving, we don't want to send joystick update
    if (axisIndex == throttle1->getAxisIndex()) {
        return throttle1->canSendJoyValue();
    } else if (axisIndex == throttle2->getAxisIndex()) {
        return throttle2->canSendJoyValue();
    }
    return true;
}

void SimDataDriver::motorStoppedAtPosition(int motorIndex) {
    if (motorIndex == throttle1->getMotorIndex()) {
        throttle1->motorStoppedAtPosition();
    } else if (motorIndex == throttle2->getMotorIndex()) {
        throttle2->motorStoppedAtPosition();
    } else if (motorIndex == trim->getMotorIndexInd1()) {
        trim->motorStoppedAtPosition();
    }
}

driver_state *SimDataDriver::getState(int index) {
    if (index == 0) {
        return throttle1->getState();
    } else if (index == 1) {
        return throttle2->getState();
    } else if (index == 2) {
        return trim->getState();
    }
    return nullptr;
}