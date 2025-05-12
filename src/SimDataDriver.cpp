#include <Arduino.h>
#include "SimDataDriver.h"
#include "context.h"
#include "state.h"
#include "config.h"
#include "Logger.h"

#define LOG_DECISIONS 
// 5 degrees of trim are 22 degrees of stepper
#define STEPPER_ANGLE_PER_TRIM_ANGLE (22.0f/5.0f)
#define TRIM_IND_VELOCITY 3
#define NEEDLE_START_MOVE_DELTA_TRESHOLD 0.005f
#define TRIM_WHEEL_FOLLOW_SPIN_MS 2000

#define PIN_END_STOP_IND1 11

#define CP_NOT_STARTED 0
#define CP_SEEKING_1_ON 1
#define CP_SEEKING_1_OVERRUN 2
#define CP_SEEKING_1_OFF 3
#define CP_SEEKING_2_ON 4
#define CP_SEEKING_2_OVERRUN 5
#define CP_SEEKING_2_OFF 6
#define CP_DONE 7

#define END_STOP_POSITION_TRIM_1 4.0f
#define END_STOP_POSITION_TRIM_2 4.0f
#define CALIBRATED_NEW_POS 5.0f

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

void TrimDriver::setup() {
    trimWheelStopTask.set(TASK_IMMEDIATE, TASK_ONCE, [&]() { 
        ctx()->motorsController.getMotor(motorIndexTrimWheel)->turnBySpeed(0);
        ctx()->motorsController.getMotor(motorIndexTrimWheel)->disable();
     });
    ctx()->taskScheduler.addTask(trimWheelStopTask);
    calibrationTask.set(50, TASK_FOREVER, [&]() { 
        calibrationTaskCallback();
     });
    ctx()->taskScheduler.addTask(calibrationTask);
    // this is workaround, end stop for indicator 1 should be part of multiplexed input pins
    // but because of hardware failure it was wired to free ESP32 pin directly
    pinMode(PIN_END_STOP_IND1, INPUT_PULLUP);
}

bool TrimDriver::isEndStop2On() {
    return !digitalRead(PIN_END_STOP_IND1);
};

bool TrimDriver::isEndStop1On() {
    return (ctx()->state.transient.getButtonsRawValue() & (1 << 5));
};


void TrimDriver::calibrate() {
    logger.log("Starting trim indicator calibration");
    // stop both motors to be sure
    ctx()->motorsController.getMotor(motorIndexInd1)->stopMotor();
    ctx()->motorsController.getMotor(motorIndexInd2)->stopMotor();
    // step 1 - move indicator to endstop1 ON position
    // if already there, don't turn the motor and go to next phase
    // if not, start the motor
    if (!isEndStop1On()) {
        ctx()->motorsController.getMotor(motorIndexInd1)->enable();
        ctx()->motorsController.getMotor(motorIndexInd1)->turnBySpeed(-5);
        logger.log("Trim indicator calibration - seeking for 1 ON position");
        calibrationPhase = CP_SEEKING_1_ON;
    } else {
        calibrationPhase = CP_SEEKING_1_OVERRUN; // we're in save ON zone already
    }
    calibrationTask.enable();
    calibrationStartTime = millis();
}

void TrimDriver::calibrationTaskCallback() {
    if (millis() - calibrationStartTime > 10000) {
        // step taking too long, probably end switch is not working, cancel that
        logger.log("Trim indicator calibration taking too long, failing");
        ctx()->motorsController.getMotor(motorIndexInd1)->stopMotor();
        ctx()->motorsController.getMotor(motorIndexInd2)->stopMotor();
        calibrationPhase = CP_NOT_STARTED;
        calibrationTask.disable();
    }
    if (calibrationPhase == CP_SEEKING_1_ON) {
        if (!isEndStop1On()) {
            // just wait until motor gets there
            return;
        }
        calibrationTask.delay(1000); // run motor 1 more second to don't stop exactly on switch trashold
        calibrationPhase = CP_SEEKING_1_OVERRUN;
        logger.log("Trim indicator calibration - runing over switch 1 position");
    } else if (calibrationPhase == CP_SEEKING_1_OVERRUN) {
        // now we have it on, so move backwards to find off position
        ctx()->motorsController.getMotor(motorIndexInd1)->enable();
        ctx()->motorsController.getMotor(motorIndexInd1)->turnBySpeed(5);
        calibrationStartTime = millis();
        logger.log("Trim indicator calibration - seeking for 1 OFF position");
        calibrationPhase = CP_SEEKING_1_OFF;
    } else if (calibrationPhase == CP_SEEKING_1_OFF) {
        if (isEndStop1On()) {
            // just wait until motor gets there
            return;
        }
        ctx()->motorsController.getMotor(motorIndexInd1)->stopMotor();
        // found the point with known value, record it as current position 
        lastActionValue = CALIBRATED_NEW_POS;
        ctx()->state.transient.getXplData()->trim = lastActionValue;
        // and move to know position as both needles must have same value
        float trimAngleChange = CALIBRATED_NEW_POS - END_STOP_POSITION_TRIM_1;
        float stepperAngleChange = trimAngleChange * STEPPER_ANGLE_PER_TRIM_ANGLE;
        ctx()->motorsController.getMotor(motorIndexInd1)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        // do the same for indicator 2
        if (!isEndStop2On()) {
            ctx()->motorsController.getMotor(motorIndexInd2)->enable();
            ctx()->motorsController.getMotor(motorIndexInd2)->turnBySpeed(-5);
            logger.log("Trim indicator calibration - seeking for 2 ON position");
        }
        calibrationPhase = CP_SEEKING_2_ON;
    } else if (calibrationPhase == CP_SEEKING_2_ON) {
        if (!isEndStop2On()) {
            // just wait until motor gets there
            return;
        }
        calibrationTask.delay(1000); // run motor 1 more second to don't stop exactly on switch treshold
        calibrationPhase = CP_SEEKING_2_OVERRUN;
        logger.log("Trim indicator calibration - runing over switch 2 position");
    } else if (calibrationPhase == CP_SEEKING_2_OVERRUN) {
        // now we have it on, so move backwards to find off position
        ctx()->motorsController.getMotor(motorIndexInd2)->enable();
        ctx()->motorsController.getMotor(motorIndexInd2)->turnBySpeed(5);
        logger.log("Trim indicator calibration - seeking for 2 OFF position");
        calibrationPhase = CP_SEEKING_2_OFF;
    } else if (calibrationPhase == CP_SEEKING_2_OFF) {
        if (isEndStop2On()) {
            // just wait until motor gets there
            return;
        }
        ctx()->motorsController.getMotor(motorIndexInd2)->stopMotor();
        // found the point with known value, record it as current position
        // found the point with known value, record it as current position 
        lastActionValue = CALIBRATED_NEW_POS;
        ctx()->state.transient.getXplData()->trim = lastActionValue;
        // and move to know position as both needles must have same value
        float trimAngleChange = CALIBRATED_NEW_POS - END_STOP_POSITION_TRIM_2;
        float stepperAngleChange = trimAngleChange * STEPPER_ANGLE_PER_TRIM_ANGLE;
        ctx()->motorsController.getMotor(motorIndexInd2)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        calibrationPhase = CP_DONE;
        calibrationTask.disable();
    }
}

void TrimDriver::trimChanged(float oldValue, float newValue) {
    if (calibrationPhase > CP_NOT_STARTED && calibrationPhase < CP_DONE) {
        return;
    }
    #ifdef LOG_DECISIONS
        char message1[200];
        sprintf(message1, "TRIM_MESSAGE Sim trim changed to %f.", newValue);
        logger.log(message1);
    #endif
    state.requestedValue = newValue;
    if (oldValue < 0 || lastActionValue < 0) {
        // ignore, this is initial value, let's wait for real delta
        lastActionValue = newValue;
        trimWheelStopTask.enable();
        return;
    }
    float trimAngleChange = newValue - lastActionValue;
    if (abs(trimAngleChange) > NEEDLE_START_MOVE_DELTA_TRESHOLD) {
        state.controlMode = CHASE;
        float stepperAngleChange = trimAngleChange * STEPPER_ANGLE_PER_TRIM_ANGLE;
        #ifdef LOG_DECISIONS
        char message[200];
        sprintf(message, "Sim trim changed by %f, ordering stepper angle change by %f.", trimAngleChange, stepperAngleChange);
        logger.log(message);
        #endif
        ctx()->motorsController.getMotor(motorIndexInd1)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        ctx()->motorsController.getMotor(motorIndexInd2)->addSteps(stepperAngleChange, TRIM_IND_VELOCITY);
        if (ctx()->state.persisted.enableTrimWheel) {
            auto wheelVelocity = trimAngleChange < 0 ? ctx()->state.persisted.trimWheelVelocity : -ctx()->state.persisted.trimWheelVelocity;
            ctx()->motorsController.getMotor(motorIndexTrimWheel)->enable();
            ctx()->motorsController.getMotor(motorIndexTrimWheel)->turnBySpeed(wheelVelocity);
            trimWheelStopTask.restartDelayed(TRIM_WHEEL_FOLLOW_SPIN_MS);
        }
        lastActionValue = newValue;
    } else if (oldValue == newValue) {
        if (trimWheelStopTask.isEnabled()) { // means wheel is turning
            trimWheelStopTask.disable(); // and stop wheel now
            ctx()->motorsController.getMotor(motorIndexTrimWheel)->turnBySpeed(0);
            ctx()->motorsController.getMotor(motorIndexTrimWheel)->disable();
        }
    }
    #ifdef LOG_DECISIONS
    else {
        char message[200];
        sprintf(message, "Trim not moved, old value %f, new value %f, last action value %f, delta %f.", oldValue, newValue, lastActionValue, trimAngleChange);
        logger.log(message);
    }
    #endif
}

void TrimDriver::motorStoppedAtPosition() {
    state.controlMode = FREE;
}

driver_state *TrimDriver::getState() {
    state.currentValue = lastActionValue;
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
    // call trim changed even with the same value on every message
    // on same values handler will stop wheel movement
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

void SimDataDriver::setup() {
    trim->setup();
};

void SimDataDriver::calibrate() {
    trim->calibrate();
};