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
    state += "}";
    return state;
}