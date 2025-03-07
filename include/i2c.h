#pragma once
#include <Wire.h>
#include <TCA9548.h>

#define PERIPHERALS 0
#define SENSORS 1

class I2cController
{
private:
    TwoWire *i2c[2]; 
    TCA9548 *i2cMultiplexer;
    boolean scanning[2] = {false, false};
public:
    void setup();
    void channel(uint8_t channel);
    TwoWire *sensors() { return i2c[SENSORS]; };
    TwoWire *peripherals() { return i2c[PERIPHERALS]; };
    void scanI2CBus(uint8_t bus);
    boolean isScanning(uint8_t bus) { return scanning[bus]; };
};
