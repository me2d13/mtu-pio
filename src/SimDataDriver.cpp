#include <Arduino.h>
#include "SimDataDriver.h"
#include "context.h"
#include "state.h"
#include "config.h"
#include "Logger.h"

#define DONT_LOG_DECISIONS 

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
    }
}

driver_state *SimDataDriver::getState(int index) {
    if (index == 0) {
        return throttle1->getState();
    } else if (index == 1) {
        return throttle2->getState();
    }
    return nullptr;
}