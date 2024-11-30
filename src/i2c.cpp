#include <Arduino.h>
#include <Wire.h>
#include "Logger.h"
#include "config.h"
#include <string>

// I2C address range
#define I2C_MIN_ADDRESS 0x08
#define I2C_MAX_ADDRESS 0x77



void scanI2CBus(TwoWire& i2c) {
    logger.log("Starting I2C bus");
    uint8_t devicesFound = 0;

    logger.log("Scanning I2C bus. Devices found:");
    for (uint8_t address = I2C_MIN_ADDRESS; address <= I2C_MAX_ADDRESS; address++) {
        i2c.beginTransmission(address);
        uint8_t error = i2c.endTransmission();

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
}
