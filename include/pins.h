#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include <Wire.h>

#define PIN_OUT_PARK_LED 0

class MultiplexedPins
{
private:
    Task pinsInitTask;
    TwoWire *wire;
    uint16_t outValue = 0xFFFF;
public:
    void setup();
    void scheduleSetup(TwoWire &wire, unsigned long delay);
    void setPin(uint8_t pin, uint8_t value);
	uint8_t read(uint8_t addr, uint8_t i2cAddress = 0);
	void write(uint8_t addr, uint8_t value, uint8_t i2cAddress = 0);
};
