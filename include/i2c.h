#pragma once
#include <Wire.h>
#include <TCA9548.h>
#include <TaskSchedulerDeclarations.h>

#define PERIPHERALS 0
#define SENSORS 1

class I2cController
{
private:
    TwoWire *i2c[2]; 
    TCA9548 *i2cMultiplexer;
    int scanningAddr[2] = {0, 0};
    uint8_t devicesFound = 0;
public:
    Task scanTask;
    void setup();
    void channel(uint8_t channel);
    TwoWire *sensors() { return i2c[SENSORS]; };
    TwoWire *peripherals() { return i2c[PERIPHERALS]; };
    void startScan(uint8_t bus);
    boolean scanStep();
    boolean isScanning(uint8_t bus) { return scanningAddr[bus] > 0; };
};
