#include <Arduino.h>
#include "pins.h"
#include "Logger.h"
#include "context.h"

#define MCP23X17_ADDRESS  0x20

// registers port A
#define MCP23X17_IODIRA   0x00
#define MCP23X17_IPOLA    0x02
#define MCP23X17_GPINTENA 0x04
#define MCP23X17_DEFVALA  0x06
#define MCP23X17_INTCONA  0x08
#define MCP23X17_IOCONA   0x0A
#define MCP23X17_GPPUA    0x0C
#define MCP23X17_INTFA    0x0E
#define MCP23X17_INTCAPA  0x10
#define MCP23X17_GPIOA    0x12
#define MCP23X17_OLATA    0x14

// registers port B
#define MCP23X17_IODIRB   0x01
#define MCP23X17_IPOLB    0x03
#define MCP23X17_GPINTENB 0x05
#define MCP23X17_DEFVALB  0x07
#define MCP23X17_INTCONB  0x09
#define MCP23X17_IOCONB   0x0B
#define MCP23X17_GPPUB    0x0D
#define MCP23X17_INTFB    0x0F
#define MCP23X17_INTCAPB  0x11
#define MCP23X17_GPIOB    0x13
#define MCP23X17_OLATB    0x15

#define MCP23X17_INT_ERR  0xFF

#define OUTPUT_ADDRESS    0x01

void MultiplexedPins::setup()
{
    write(MCP23X17_IODIRA, 0x00, OUTPUT_ADDRESS); // set all pins to output
    write(MCP23X17_IODIRB, 0x00, OUTPUT_ADDRESS); // set all pins to output
    logger.log("MCP23017 pin directions set");
}

void MultiplexedPins::scheduleSetup(TwoWire &wire, unsigned long delay)
{
    this->wire = &wire;
    pinsInitTask.set(TASK_IMMEDIATE, TASK_ONCE, [&]() { setup(); });
    ctx()->taskScheduler.addTask(pinsInitTask);
    pinsInitTask.enableDelayed(delay);
    logger.log("MCP23017 setup planned");
}

void MultiplexedPins::setPin(uint8_t pin, uint8_t value)
{
    bitWrite(outValue, pin, value);
    if (pin < 8) {
        write(MCP23X17_OLATA, outValue, OUTPUT_ADDRESS);
    } else {
        write(MCP23X17_OLATB, outValue >> 8, OUTPUT_ADDRESS);
    }
    String message = "MCP23017 set pin: " + String(pin) + " to value: " + String(value) + " - outValue: " + String(outValue, BIN);
    logger.log(message.c_str());
}

uint8_t MultiplexedPins::read(uint8_t addr, uint8_t i2cAddress) {
    wire->beginTransmission(MCP23X17_ADDRESS | i2cAddress);
	wire->write(addr);
	wire->endTransmission();
	wire->requestFrom(MCP23X17_ADDRESS | i2cAddress, 1);
    auto value = wire->read();
    String message = "MCP23017 read value: " + String(value) + " from address: " + String(addr);
    logger.log(message.c_str());
    return value;
}

void MultiplexedPins::write(uint8_t addr, uint8_t value, uint8_t i2cAddress) {
	wire->beginTransmission(MCP23X17_ADDRESS | i2cAddress);
	wire->write(addr);
	wire->write(value);
	wire->endTransmission();
}
