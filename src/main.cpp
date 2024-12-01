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
#include "joy.h"
#include "axis.h"

using namespace reactesp;

EventLoop event_loop;

TwoWire i2Cone = TwoWire(0);
TwoWire i2Ctwo = TwoWire(1);

int joyTest = 0;

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
    //delay(1000);
    i2Ctwo.begin(SDA_PIN_SENSORS, SCL_PIN_SENSORS);
    //scanI2CBus(i2Cone);
    //scanI2CBus(i2Ctwo);
    setupLcd(i2Cone);
    setupAxis(i2Ctwo);
    lcdAbout();

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    setupWeb();
    setupJoy();

    Serial.println("\n##################################");
    Serial.println(F("ESP32 Information:"));
    Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n\r", ESP.getHeapSize(), ESP.getHeapSize()-ESP.getFreeHeap(), ESP.getFreeHeap());
    Serial.printf("Sketch Size %d, Free Sketch Space %d\n\r", ESP.getSketchSize(), ESP.getFreeSketchSpace());
    Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n\r", ESP.getPsramSize(), ESP.getFreePsram());
    Serial.printf("Chip Model %s, ChipRevision %d, Cpu Freq %d, SDK Version %s\n\r", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
    Serial.printf("Flash Size %d, Flash Speed %d\n\r", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
    Serial.println("##################################\n");

    event_loop.onRepeat(1000, [] () {
      //lcdAbout();
      //Serial.print("Angle: ");
      //Serial.println(getAngle());
    });

    
    event_loop.onRepeat(100, [] () {
      readAxisData();
      setJoyAxis(X_AXIS, getAxisValue(0));
      setJoyAxis(Y_AXIS, joyTest);
      sendJoy();
      lcdAbout();
      if (++joyTest > 4095) joyTest = 0;
      //Serial.print("Angle: ");
      //Serial.println(getAngle());
    });
}

void loop() {
  event_loop.tick();
}