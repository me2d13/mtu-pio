#include <Arduino.h>
#include "state.h"
#include "config.h"
#include "context.h"
#include "Logger.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

axis_settings defaultAxisSettings[NUMBER_OF_AXIS] = {
    {274, 93, "SB", false},
    {3443, 1043, "THR1", true},
    {3945, 2265, "THR2", false},
    {0, 4096, "FLAPS", false},
    {0, 4096, "TRIM", false}
};
motor_settings defaultMotorSettings[MOTORS_COUNT] = {
    {400, 4, "THR1"},
    {400, 4, "THR2"},
    {100, 4, "SB"},
    {100, 1, "TRIM"},
    {100, 1, "TRI1"},
    {100, 1, "TRI2"}
};

PersistedState::PersistedState() {
    resetToDefaultValues();
}

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
    JsonDocument doc;
    fillJsonDocument(doc);    
    String state;
    serializeJson(doc, state);
    return state;
}

void PersistedState::fillJsonDocument(JsonDocument &doc) {
    JsonArray axisSettingsArray = doc["axisSettings"].to<JsonArray>();
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        JsonObject axisSetting = axisSettingsArray.add<JsonObject>();
        axisSetting["name"] = axisSettings[i].name;
        axisSetting["minValue"] = axisSettings[i].minValue;
        axisSetting["maxValue"] = axisSettings[i].maxValue;
        axisSetting["isReversed"] = axisSettings[i].isReversed;
    }

    JsonArray motorSettingsArray = doc["motorSettings"].to<JsonArray>();
    for (int i = 0; i < MOTORS_COUNT; i++) {
        JsonObject motorSetting = motorSettingsArray.add<JsonObject>();
        motorSetting["name"] = motorSettings[i].name;
        motorSetting["runCurrent"] = motorSettings[i].runCurrent;
        motorSetting["microSteps"] = motorSettings[i].microSteps;
    }
}

String PersistedState::loadFromJsonDocument(JsonDocument &doc) {
    if (doc.isNull()) {
        return "Error: Document is null";
    }
    if (!doc.is<JsonObject>()) {
        return "Error: Document is not a JSON object";
    }
    logger.log("Loading state from JSON document");

    JsonObject root = doc.as<JsonObject>();
    return loadFromJsonObject(root, false);
}

String PersistedState::loadFromJsonObject(JsonObject &root, boolean saveOnSuccess) {
    boolean changesDone = false;
    JsonArray axisSettingsArray = root["axisSettings"];
    if (axisSettingsArray) {
        if (axisSettingsArray.size() != NUMBER_OF_AXIS) {
            return "Error: Invalid number of axis settings. Load interrupted.";
        }
        for (int i = 0; i < NUMBER_OF_AXIS; i++) {
            JsonObject axisSetting = axisSettingsArray[i];
            axisSettings[i].minValue = axisSetting["minValue"];
            axisSettings[i].maxValue = axisSetting["maxValue"];
            axisSettings[i].isReversed = axisSetting["isReversed"];
        }
        changesDone = true;
        logger.log("Axis settings loaded successfully");
    } else {
        logger.log("Skipped: axisSettings array not found in JSON document");
    }

    JsonArray motorSettingsArray = root["motorSettings"];
    if (motorSettingsArray) {
        if (motorSettingsArray.size() != MOTORS_COUNT) {
            return "Error: Invalid number of motor settings. Load interrupted.";
        }
        for (int i = 0; i < MOTORS_COUNT; i++) {
            JsonObject motorSetting = motorSettingsArray[i];
            motorSettings[i].runCurrent = motorSetting["runCurrent"];
            motorSettings[i].microSteps = motorSetting["microSteps"];
        }
        changesDone = true;
        logger.log("Motor settings loaded successfully");
    } else {
        logger.log("Skipped: motorSettings array not found in JSON document");
    }
    if (changesDone && saveOnSuccess) {
        logger.log("Changes applied to configuration, saving to flash memory");
        saveToFlash();
    } else if (!changesDone) {
        logger.log("No changes made, not saving to flash memory");
    } else {
        logger.log("No save of configuration to flash memory needed");
    }
    return "Success: State loaded from JSON document";
}

String PersistedState::loadFromJson(String json) {
    auto inputLength = json.length();
    logger.log(("Loading state from JSON string of length: " + String(inputLength)).c_str());
    if (inputLength == 0) {
        return "Empty JSON string, nothing loaded";
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return "Error: Failed to parse JSON document";
    }
    return loadFromJsonDocument(doc);
}

void PersistedState::factoryReset() {
    logger.log("Factory reset was called");
    resetToDefaultValues();
 }

void PersistedState::resetToDefaultValues() {
    // called from contructor, so no logging and fancy code here
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        axisSettings[i] = defaultAxisSettings[i];
    }
    for (int i = 0; i < MOTORS_COUNT; i++) {
        motorSettings[i] = defaultMotorSettings[i];
    }
}

void PersistedState::saveToFlash() {
    logger.log("Saving persisted state to flash");
    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        logger.log("Error: Failed to open file for writing");
        return;
    }
    JsonDocument doc;
    fillJsonDocument(doc);
    String jsonString;
    serializeJson(doc, jsonString);
    file.print(jsonString);
    file.close();
    logger.log("Persisted state saved to flash successfully");
}

void PersistedState::loadFromFlash() {
    logger.log("Loading persisted state from flash");
    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        logger.log("Error: Failed to open /config.json file for reading");
        return;
    }
    String jsonString = file.readString();
    file.close();
    String result = loadFromJson(jsonString);
    logger.log(result.c_str());
}