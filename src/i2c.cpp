#include <Arduino.h>
#include <Wire.h>
#include "Logger.h"
#include "config.h"
#include <string>
#include "Logger.h"
#include <TCA9548.h>
#include "i2c.h"
#include <TaskSchedulerDeclarations.h>
#include "context.h"

// I2C address range
#define I2C_MIN_ADDRESS 0x08
#define I2C_MAX_ADDRESS 0x77


TwoWire _i2C1 = TwoWire(0); // all peripherals (LCD, 2017)
TwoWire _i2C2 = TwoWire(1); // sensors

void scanTaskCallback() {
    boolean shouldContinue = ctx()->i2c()->scanStep();
    if (!shouldContinue) {
        logger.log("Disabling I2C scan task - no more addresses to scan");
        ctx()->i2c()->scanTask.disable();
    }
}

void I2cController::setup() {
    i2c[0] = &_i2C1;
    i2c[1] = &_i2C2;

    i2c[0]->begin(SDA_PIN_LCD, SCL_PIN_LCD);
    i2c[1]->begin(SDA_PIN_SENSORS, SCL_PIN_SENSORS);
    //scanI2CBus(_i2C1);
    //scanI2CBus(i2C2);
    //delay(1000);
    i2cMultiplexer = new TCA9548(0x70, i2c[0]);
    if (i2cMultiplexer->begin() == false)
    {
        logger.log("An Error has occurred while initializing I2C Multiplexer");
        ctx()->state.transient.incrementI2cChannelSwitchFailures();
    } else {
        logger.log("I2C Multiplexer initialized");
    }
    scanTask.setInterval(TASK_IMMEDIATE);
    scanTask.setIterations(TASK_FOREVER);
    scanTask.setCallback(scanTaskCallback);
    ctx()->taskScheduler.addTask(scanTask);
}

boolean I2cController::scanStep() {
    for (uint8_t busIndex = 0; busIndex < 2; busIndex++) {
        if (scanningAddr[busIndex] > 0) {
            uint8_t address = scanningAddr[busIndex];
            // Serial.println("Scanning address: " + String(address, HEX));
            i2c[busIndex]->beginTransmission(address);
            uint8_t error = i2c[busIndex]->endTransmission();
            std::string logMessage = "";
            if (error == 0) {
                logMessage += "Device found at address 0x";
                devicesFound++;
            } else if (error == 4) {
                logMessage += "Unknown error at address 0x";
            }
            if (error == 0 || error == 4) {
                if (address < 16) logMessage += "0"; // Leading zero for single-digit addresses
                logMessage += String(address, HEX).c_str();
                logger.log(logMessage);
            }
            scanningAddr[busIndex]++;
            if (scanningAddr[busIndex] > I2C_MAX_ADDRESS) {
                scanningAddr[busIndex] = 0;
                if (devicesFound == 0) {
                    logger.log("No I2C devices found");
                }
                return false;
            }
            return true;
        }
    }
    return false;
}

void I2cController::startScan(uint8_t busIndex) {
    logger.log("Enabling I2C scan task");
    scanningAddr[busIndex] = I2C_MIN_ADDRESS;
    devicesFound = 0;
    logger.log("Scanning I2C bus. Devices found:");
    scanTask.enable();
}

void I2cController::channel(uint8_t channel) {
    // logger.log(("Switching I2C channel to " + String(channel)).c_str());
    bool success = i2cMultiplexer->selectChannel(channel);
    if (!success) {
        ctx()->state.transient.incrementI2cChannelSwitchFailures();
    }
}