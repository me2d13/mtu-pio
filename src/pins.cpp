#include <Arduino.h>
#include "pins.h"
#include "Logger.h"
#include "context.h"
#include "state.h"
#include "joy.h"

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

#define INPUT_ADDRESS     0x00
#define OUTPUT_ADDRESS    0x01

Task inputPinsPollingTask;
Task readInputPinsTask;

int last12VState = 3;

void pollPins();

void MultiplexedPins::setup()
{
    // output multiplexer
    write(MCP23X17_IODIRA, 0x00, OUTPUT_ADDRESS); // set all pins to output
    write(MCP23X17_IODIRB, 0x00, OUTPUT_ADDRESS); // set all pins to output
    // initialize output pins with outValue
    write(MCP23X17_OLATA, outValue, OUTPUT_ADDRESS); // set all pins to output
    write(MCP23X17_OLATB, outValue >> 8, OUTPUT_ADDRESS); // set all pins to output
    // input multiplexer
    write(MCP23X17_IODIRA, 0xFF, INPUT_ADDRESS); // set all pins to input
    write(MCP23X17_IODIRB, 0xFF, INPUT_ADDRESS); // set all pins to input
    // set all input pins to support interrupt on change (compared with previous value)
    // 1) enable inpterupts
    write(MCP23X17_GPINTENA, 0xFF, INPUT_ADDRESS); // set all pins to interrupt on change
    write(MCP23X17_GPINTENB, 0xFF, INPUT_ADDRESS); // set all pins to interrupt on change
    // 2) set mode to compare with previous value via INTCONA/B
    write(MCP23X17_INTCONA, 0x00, INPUT_ADDRESS); // set all pins to compare with previous value
    write(MCP23X17_INTCONB, 0x00, INPUT_ADDRESS); // set all pins to compare with previous value
    // 3) set IOCON so INTB indicates any change on GPIOA and GPIOB
    // IOCON has bits: BANK MIRROR SEQOP DISSLW HAEN ODR INTPOL UNUSED
    write(MCP23X17_IOCONA, 0b01000000, INPUT_ADDRESS); // set IOCON to default value
    // 4) set pullups on all input pins
    write(MCP23X17_GPPUA, 0xFF, INPUT_ADDRESS); // set all pins to pullup
    write(MCP23X17_GPPUB, 0xFF, INPUT_ADDRESS); // set all pins to pullup

    logger.log("MCP23017 pin directions set");
    pinMode(PIN_12V_IN, INPUT_PULLUP); // set 12V detect pin to input with pullup
    // configure interrupt output from MCP23017 as input pin
    pinMode(PIN_BUTTONS_INTERRUPT, INPUT_PULLUP); 
    // attach interrupt handler to PIN_BUTTONS_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTONS_INTERRUPT), []() {
        readInputPinsTask.restartDelayed(50); // 50 for debounce time
    }, FALLING); // trigger on falling edge as INTB is active low (IOCON.INTPOL = 0)
    //TODO: this should be pulled out of multiplexer class
    inputPinsPollingTask.setInterval(500);
    inputPinsPollingTask.setIterations(TASK_FOREVER);
    inputPinsPollingTask.setCallback(pollPins);
    ctx()->taskScheduler.addTask(inputPinsPollingTask);
    inputPinsPollingTask.enableDelayed(500);

    // task runs every minute, but is forced by interrupts to run immediately
    readInputPinsTask.setInterval(60*1000);
    readInputPinsTask.setIterations(TASK_FOREVER);
    readInputPinsTask.setCallback([]() {
        ctx()->pins.readInputPins();
    });
    ctx()->taskScheduler.addTask(readInputPinsTask);
    readInputPinsTask.enableDelayed(1000);
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
    //String message = "MCP23017 read value: " + String(value) + " from address: " + String(addr);
    //logger.log(message.c_str());
    return value;
}

void MultiplexedPins::write(uint8_t addr, uint8_t value, uint8_t i2cAddress) {
	wire->beginTransmission(MCP23X17_ADDRESS | i2cAddress);
	wire->write(addr);
	wire->write(value);
	wire->endTransmission();
}

void MultiplexedPins::readInputPins() {
    uint8_t valueA = read(MCP23X17_GPIOA, INPUT_ADDRESS);
    uint8_t valueB = read(MCP23X17_GPIOB, INPUT_ADDRESS);
    int rawValue = (valueB | (valueA << 8)) ^ 0xFFFF; // invert value as pullups are used
    ctx()->state.transient.setButtonsRawValue(rawValue);
    if (ENABLE_JOYSTICK) {
        readStateDataAndSendJoy();
    }
    // handle parking LED - for now just hardcoded based on switch state
    setParkingBrakeIndicator(rawValue & (1 << 4));
    String message = "MCP23017 read input pins: " + String(valueA, BIN) + " " + String(valueB, BIN);
    logger.log(message.c_str());
}

void MultiplexedPins::setParkingBrakeIndicator(bool value) {
    setPin(PIN_OUT_PARK_LED, value ? HIGH : LOW);
}

void pollPins() {
    int current12VState = digitalRead(PIN_12V_IN);
    if (current12VState != last12VState) {
        if (current12VState == LOW) {
            logger.log("12V power detected");
            ctx()->state.transient.set12Vpresent(true);
            if (last12VState == HIGH) {
                logger.log("Restarting LCD as 12V power was lost and now is present again");
                // 12V was lost and now is present again (is not initial state when last12VState == 3)
                // in this case re-initalize LCD
                ctx()->screenController.hwSetup();
                ctx()->screenController.render();
                // and also re-initialize motors
                ctx()->motorsController.reInit();
            }
        } else {
            logger.log("12V power lost");
            ctx()->state.transient.set12Vpresent(false);
        }
        last12VState = current12VState;
    }
}