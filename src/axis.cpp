#include <Arduino.h>
#include "axis.h"
#include <AS5600.h>
#include "joy.h"
#include "config.h"
#include "context.h"
#include "Logger.h"

u_int8_t axisI2cIndexes[] = {1};

void AxesController::setup() {
    ctx()->i2c()->channel(axisI2cIndexes[0]);
    sensor = new AS5600(ctx()->i2c()->peripherals());
    if (sensor->begin()) {
        logger.log("Axis setup done OK");
    } else {
        logger.log("Axis setup failed");
    }
    axesCheckTask.set(10, TASK_FOREVER, [&]() { readAxisData(); });
    ctx()->taskScheduler.addTask(axesCheckTask);
    axesCheckTask.enableDelayed(1000); // 1 second delay for setup
    logger.log("Axis reading task scheduled");
    measureIntervalStart = millis();
}


void AxesController::readAxisData() {
    uint32_t currentTime = millis();
    // iterate over all axis
    for (int i = 0; i < 1 /*NUMBER_OF_AXIS*/; i++) {
        ctx()->i2c()->channel(axisI2cIndexes[i]);
        int value = sensor->rawAngle();
        if (sensor->lastError() == AS5600_OK) {
            axisValues[i] = value;
        } else {
            axisValues[i] = -1; // error value
        }
    }
    measuredSamples++;
    measureTimeSpent += millis() - currentTime;
    if (currentTime - measureIntervalStart >= 10000) { // 10 seconds interval
        float samplesPerSecond = (float)measuredSamples / 10.0;
        float averageTime = (float)measureTimeSpent / (float)measuredSamples;
        measureTimeSpent = 0;
        String message = "Axis samples per second: " + String(samplesPerSecond) + " - average time: " + String(averageTime) + " ms - measured samples: " + String(measuredSamples);
        if (averageTime > 100) {
            message += " - WARNING: Axis reading time is too high!";
        }
        logger.log(message.c_str());
        measureIntervalStart = currentTime;
        measuredSamples = 0;
    }
}

int AxesController::getAxisValue(int index) {
    return axisValues[index];
}