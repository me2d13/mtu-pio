#include "context.h"
#include "SPIFFS.h"
#include "Logger.h"
#include "config.h"
#include "web.h"
#include "net.h"
#include "i2c.h"
#include <Wire.h>
#include "lcd.h"
#include <ReactESP.h>
#include "joy.h"
#include "axis.h"
#include "motor.h"
#include "api.h"
#include <ESPAsyncWebServer.h>

GlobalContext instance;

GlobalContext::GlobalContext() {};

// temporarly
#define STEP_PIN 17
#define DIR_PIN 18

Motor *testMotor;

void GlobalContext::setup()
{
    setupWifi();
    logger.log("IP Address: " + getIp());
    syncNtp();

    i2cController = new I2cController();
    i2cController->setup();

    setupLcd();
    logger.log("LCD initialized");
    setupAxis();
    logger.log("Axis initialized");
    //lcdAbout();

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        logger.log("An Error has occurred while mounting SPIFFS");
    }

    server = setupWeb();
    ApiController apiController(this);
    setupJoy();
    setupMotors();

    testMotor = new Motor(0, STEP_PIN, DIR_PIN);
    testMotor->init();
    instance.eventLoop.onDelay(10000, []()
                               {
                                   testMotor->makeSteps(90, 20);
                                   // testMotor->turnBySpeed(100);
                               });
}

AsyncWebServer *GlobalContext::getServer()
{
    return server;
}

GlobalContext *ctx()
{
    return &instance;
}