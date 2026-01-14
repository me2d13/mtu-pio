#include "context.h"
#include "SPIFFS.h"
#include "Logger.h"
#include "config.h"
#include "web.h"
#include "net.h"
#include "i2c.h"
#include <Wire.h>
#include "Screen.h"
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

GlobalContext::GlobalContext() : screenController() {};

EncoderInput rotaryEncoder; // doesn't have to be in .h file as it would introduce multiple implementation errors of attachInterrupt

const char *LOG_TAG = "MTUcontext";

void GlobalContext::setup()
{
    unsigned long startTime = millis();
    i2cController = new I2cController();
    i2cController->setup();
    screenController.hwSetup();
    logger.log("Starting up...");
    if (ENABLE_NETWORK) {
        screenController.showText("Setting WIFI...");
        setupWifi();
        logger.log("IP Address: " + getIp());
        screenController.showText("Syncing time...");
        syncNtp();
        screenController.showText("Setting OTA...");
        setupOTA();
    }

    pins.scheduleSetup(*(i2cController->peripherals()), 100);

    screenController.showText("Loading config...");
    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        logger.log("An Error has occurred while mounting SPIFFS");
    }
    state.persisted.loadFromFlash();
    
    screenController.menu();
    axesController.setup();
    logger.log("Axis initialized");

    if (ENABLE_HTTP_SERVER && ENABLE_NETWORK) {
        server = setupWeb();
        apiController = new ApiController(this);
        apiController->setup();
    }
    if (ENABLE_JOYSTICK) {
        setupJoy();
    }
    if (ENABLE_UDP && ENABLE_NETWORK) {
        simUdpInterface.setup();
    }
    rotaryEncoder.setup();
    motorsController.scheduleSetup(300); // must be after pins setup
    screenController.render();
    simDataDriver.setup();
    String msg = "Setup completed in " + String(millis() - startTime) + " ms";
    logger.log(msg.c_str());
    Serial.println(msg.c_str());
    ESP_LOGI( LOG_TAG, "Setup completed in %d ms", millis() - startTime );
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