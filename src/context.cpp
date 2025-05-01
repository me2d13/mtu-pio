#include "context.h"
#include "SPIFFS.h"
#include "Logger.h"
#include "config.h"
#include "web.h"
#include "net.h"
#include "i2c.h"
#include <Wire.h>
#include "lcd.h"
#include <TaskScheduler.h>
#include "joy.h"
#include "axis.h"
#include "motor.h"
#include "api.h"
#include "pins.h"
#include "udp.h"
#include "rotary.h"

#ifdef ENABLE_HTTP_SERVER
#include <ESPAsyncWebServer.h>
#endif

GlobalContext instance;

GlobalContext::GlobalContext() {};

EncoderInput rotaryEncoder; // doesn't have to be in .h file as it would introduce multiple implementation errors of attachInterrupt

void GlobalContext::setup()
{
    setupWifi();
    logger.log("IP Address: " + getIp());
    syncNtp();
    setupOTA();

    i2cController = new I2cController();
    i2cController->setup();
    pins.scheduleSetup(*(i2cController->peripherals()), 100);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        logger.log("An Error has occurred while mounting SPIFFS");
    }
    state.persisted.loadFromFlash();
    
    setupLcd();
    axesController.setup();
    logger.log("Axis initialized");
    lcdAbout();

    if (ENABLE_HTTP_SERVER) {
        server = setupWeb();
        ApiController apiController(this);
    }
    if (ENABLE_JOYSTICK) {
        setupJoy();
    }
    if (ENABLE_UDP) {
        xplaneInterface.setup();
    }
    rotaryEncoder.setup();
    motorsController.scheduleSetup(300); // must be after pins setup
}

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

void GlobalContext::debugCall(int index)
{
    //pins.scheduleSetup(*(i2cController->peripherals()));
    //motorsController.getMotor(0)->debugCall();
    //pins.read(MCP23X17_IODIRB);
    axesController.readSingleAxis(index);
    //pins.write(MCP23X17_OLATB, 0x00);
}

AsyncWebServer *GlobalContext::getServer()
{
    return server;
}

GlobalContext *ctx()
{
    return &instance;
}