#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "Logger.h"
#include <time.h>
#include "config.h"
#include "web.h"
#include "net.h"
#include "i2c.h"
#include <Wire.h>
#include "lcd.h"
#include <ReactESP.h>

using namespace reactesp;

EventLoop event_loop;

TwoWire i2Cone = TwoWire(0);

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Add some logs
    logger.log("System initializing...");
    logger.log("Performing self-check...");
    logger.log("System ready!");

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());

    setupWifi();
    logger.log("IP Address: " + getIp());
    syncNtp();

    i2Cone.begin(SDA_PIN_LCD, SCL_PIN_LCD);
    scanI2CBus(i2Cone);
    setupLcd(i2Cone);
    lcdAbout();

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    setupWeb();

    event_loop.onRepeat(1000, [] () {
      lcdAbout();
    });
}

void loop() {
  event_loop.tick();
}