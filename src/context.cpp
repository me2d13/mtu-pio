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

GlobalContext instance;

TwoWire _i2C1 = TwoWire(0); // all peripherals (LCD, 2017)
TwoWire _i2C2 = TwoWire(1); // sensors

GlobalContext::GlobalContext() {};


// temporarly
#define STEP_PIN 17
#define DIR_PIN 18

Motor *testMotor;

void GlobalContext::setup() {
    i2C1 = &_i2C1; // all peripherals (LCD, 2017)
    i2C2 = &_i2C2; // sensors

    setupWifi();
    logger.log("IP Address: " + getIp());
    syncNtp();

    i2C1->begin(SDA_PIN_LCD, SCL_PIN_LCD);
    //delay(1000);
    i2C2->begin(SDA_PIN_SENSORS, SCL_PIN_SENSORS);
    //scanI2CBus(i2C1);
    //scanI2CBus(i2C2);
    setupLcd(*i2C1);
    setupAxis(*i2C2);
    lcdAbout();

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      logger.log("An Error has occurred while mounting SPIFFS");
    }

    setupWeb();
    setupJoy();
    setupMotors();

    testMotor = new Motor(0, STEP_PIN, DIR_PIN);
    testMotor->init();
    instance.eventLoop.onDelay(10000, []() {
        testMotor->makeSteps(90, 20);
        //testMotor->turnBySpeed(100);
    });
}

GlobalContext* ctx() {
    return &instance;
}