#include <Arduino.h>
#include <Wire.h>
#include "Logger.h"
#include "config.h"
#include <string>
#include "Logger.h"
#include <TCA9548.h>
#include "i2c.h"

// I2C address range
#define I2C_MIN_ADDRESS 0x08
#define I2C_MAX_ADDRESS 0x77


TwoWire _i2C1 = TwoWire(0); // all peripherals (LCD, 2017)
TwoWire _i2C2 = TwoWire(1); // sensors

void I2cController::setup() {
    i2c[0] = &_i2C1;
    i2c[1] = &_i2C2;

    i2c[0]->begin(SDA_PIN_LCD, SCL_PIN_LCD);
    i2c[1]->begin(SDA_PIN_SENSORS, SCL_PIN_SENSORS);
    //scanI2CBus(_i2C1);
    //scanI2CBus(i2C2);
    i2cMultiplexer = new TCA9548(0x70, i2c[0]);
    if (i2cMultiplexer->begin() == false)
    {
        logger.log("An Error has occurred while initializing I2C Multiplexer");
    } else {
        logger.log("I2C Multiplexer initialized");
    }
}

void I2cController::scanI2CBus(uint8_t busIndex) {
    logger.log("Starting I2C bus");
    uint8_t devicesFound = 0;
    scanning[busIndex] = true;

    logger.log("Scanning I2C bus. Devices found:");
    for (uint8_t address = I2C_MIN_ADDRESS; address <= I2C_MAX_ADDRESS; address++) {
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
    }
    if (devicesFound == 0) {
        logger.log("No I2C devices found");
    }
    scanning[busIndex] = false;
}

void I2cController::channel(uint8_t channel) {
    i2cMultiplexer->selectChannel(channel);
}