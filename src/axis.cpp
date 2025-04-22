#include <Arduino.h>
#include "axis.h"
#include <AS5600.h>
#include "joy.h"
#include "config.h"
#include "context.h"
#include "Logger.h"

u_int8_t axisI2cIndexes[] = {
    I2C_CHANNEL_THROTTLE_1, 
    I2C_CHANNEL_THROTTLE_2,
    I2C_CHANNEL_SPEED_BRAKE,
    I2C_CHANNEL_FLAPS,
    I2C_CHANNEL_TRIM};

void AxesController::setup() {
    if (ENABLE_SENSORS == 0) {
        logger.log("Axis setup skipped, ENABLE_SENSORS is 0");
        return;
    }
    ctx()->i2c()->channel(axisI2cIndexes[4]);
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

void AxesController::readSingleAxis(int index) {
    ctx()->i2c()->channel(axisI2cIndexes[index]);
    //delay(10); // wait for channel switch to settle
    int value = sensor->rawAngle();
    if (sensor->lastError() == AS5600_OK) {
        ctx()->state.transient.setAxisValue(index, value);
    } else {
        ctx()->state.transient.setAxisValue(index, -1); // error value
        ctx()->state.transient.incrementAxisReadFailures(index);
    }
}

void AxesController::readAxisData() {
    uint32_t currentTime = millis();
    // iterate over all axis
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        readSingleAxis(i);
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