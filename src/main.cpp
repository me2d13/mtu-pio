#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "Logger.h"
#include <time.h>
#include "config.h"
#include "web.h"
#include "net.h"


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

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    setupWeb();
}

void loop() {
  // Log something periodically
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime >= 5000) { // Log every 5 seconds
        String uptime = "Uptime: " + String(millis() / 1000) + " seconds";
        //notifyClients(logger.log(uptime.c_str()).c_str()); // Notify clients of the new log message
        lastLogTime = millis();
    }
}