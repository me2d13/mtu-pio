#pragma once
#include <Arduino.h>
#include "config.h"
#include <ArduinoJson.h>

struct axis_settings
{
    // if minValue > maxValue there are 2 posible scenarios:
    // 1. the axis is reversed and the value is in the range of minValue and maxValue
    // 2. the axis is not reversed sensor raw angle overflows from 4095 to 0
    // these 2 scenarios are identified by the isReversed flag
    int minValue;
    int maxValue;
    char name[6];
    bool isReversed;
};

struct motor_settings
{
    int runCurrent;
    int microSteps;
    char name[6];
};


class PersistedState
{
private:
    void fillJsonDocument(JsonDocument &doc);
    String loadFromJsonDocument(JsonDocument &doc);
public:
    PersistedState();
    axis_settings axisSettings[NUMBER_OF_AXIS];
    motor_settings motorSettings[MOTORS_COUNT];
    String reportState();
    String loadFromJson(String json);
    String loadFromJsonObject(JsonObject &jsonObject, boolean saveOnSuccess);
    void factoryReset(); // same as resetToDefaultValues but with more logging
    void resetToDefaultValues();
    void saveToFlash();
    void loadFromFlash();
};

class TransientState
{
private:
    int axisValues[NUMBER_OF_AXIS];
    bool twelveVPresent = false;
    int i2cChannelSwitchFailures = 0;
    int axisReadFailures[NUMBER_OF_AXIS];
    int buttonsRawValue;
    long roataryEncoderValue = 0;
    int rotaryButtonPressedTime = 0;
    int rotaryButtonPressedCount = 0;
public:
    TransientState() {
        for (int i = 0; i < NUMBER_OF_AXIS; i++) {
            axisValues[i] = 0;
            axisReadFailures[i] = 0;
        }
    }
    int getAxisValue(int index);
    int getCalibratedAxisValue(int index, axis_settings *settings);
    void setAxisValue(int index, int value);
    void set12Vpresent(bool present) { this->twelveVPresent = present; }
    bool is12Vpresent() { return this->twelveVPresent; }
    int incrementI2cChannelSwitchFailures() { return ++this->i2cChannelSwitchFailures; }
    int getI2cChannelSwitchFailures() { return this->i2cChannelSwitchFailures; }
    int incrementAxisReadFailures(int index) { return ++this->axisReadFailures[index]; }
    int getAxisReadFailures(int index) { return this->axisReadFailures[index]; }
    int getButtonsRawValue() { return this->buttonsRawValue; }
    void setButtonsRawValue(int value) { this->buttonsRawValue = value; }
    long getRotaryEncoderValue() { return this->roataryEncoderValue; }
    void setRotaryEncoderValue(long value) { this->roataryEncoderValue = value; }
    int getRotaryButtonPressedTime() { return this->rotaryButtonPressedTime; }
    void setRotaryButtonPressedTime(int value) { this->rotaryButtonPressedTime = value; }
    int getRotaryButtonPressedCount() { return this->rotaryButtonPressedCount; }
    void setRotaryButtonPressedCount(int value) { this->rotaryButtonPressedCount = value; }
    String reportState();
};

class State
{
private:
public:
    PersistedState persisted;
    TransientState transient;
};

