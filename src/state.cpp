#include <Arduino.h>
#include "state.h"
#include "config.h"
#include "context.h"
#include "Logger.h"

int TransientState::getAxisValue(int index) {
    if (index < 0 || index >= NUMBER_OF_AXIS) {
        return -1; // error value
    }
    return axisValues[index];
}

int TransientState::getCalibratedAxisValue(int index, axis_settings *settings) {
    if (index < 0 || index >= NUMBER_OF_AXIS) {
        return -1; // error value
    }
    int value = axisValues[index];
    return calculateCalibratedValue(value, settings);
}

void TransientState::setAxisValue(int index, int value) {
    if (index < 0 || index >= NUMBER_OF_AXIS) {
        return; // error value
    }
    axisValues[index] = value;
}

String TransientState::reportState() {
    // Create a JSON object to hold the state
    String state = "{";
    state += "\"axisValues\":[";
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        state += String(axisValues[i]);
        if (i < NUMBER_OF_AXIS - 1) {
            state += ",";
        }
    }
    state += "],";
    state += "\"axisCalibratedValues\":[";
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        if (axisValues[i] == -1) {
            state += "-1"; // error value
        } else {
            state += String(getCalibratedAxisValue(i, &ctx()->state.persisted.axisSettings[i]));
        }
        if (i < NUMBER_OF_AXIS - 1) {
            state += ",";
        }
    }
    state += "],";
    state += "\"buttonsRawValue\":" + String(buttonsRawValue) + ",";
    state += "\"12VPresent\":" + String(twelveVPresent ? "true" : "false") + ",";
    state += "\"i2cChannelSwitchFailures\":" + String(i2cChannelSwitchFailures) + ",";
    state += "\"axisReadFailures\":[";
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        state += String(axisReadFailures[i]);
        if (i < NUMBER_OF_AXIS - 1) {
            state += ",";
        }
    }
    state += "]}";
    return state;
}

String PersistedState::reportState() {
    // Create a JSON object to hold the state
    String state = "{";
    // Add other persisted state variables here
    state += "\"axisSettings\":[";
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        state += "{";
        state += "\"name\":\"" + String(axisSettings[i].name) + "\",";
        state += "\"minValue\":" + String(axisSettings[i].minValue) + ",";
        state += "\"maxValue\":" + String(axisSettings[i].maxValue) + ",";
        state += "\"isReversed\":" + String(axisSettings[i].isReversed ? "true" : "false");
        state += "}";
        if (i < NUMBER_OF_AXIS - 1) {
            state += ",";
        }
    }
    state += "],";
    state += "\"motorSettings\":[";
    for (int i = 0; i < MOTORS_COUNT; i++) {
        state += "{";
        state += "\"name\":\"" + String(motorSettings[i].name) + "\",";
        state += "\"runCurrent\":" + String(motorSettings[i].runCurrent) + ",";
        state += "\"microSteps\":" + String(motorSettings[i].microSteps);
        state += "}";
        if (i < MOTORS_COUNT - 1) {
            state += ",";
        }
    }
    state += "]}";
    return state;
}