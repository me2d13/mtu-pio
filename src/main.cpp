#include <Arduino.h>
#include "context.h"

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());

    Serial.println("\n##################################");
    Serial.println(F("ESP32 Information:"));
    Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n\r", ESP.getHeapSize(), ESP.getHeapSize()-ESP.getFreeHeap(), ESP.getFreeHeap());
    Serial.printf("Sketch Size %d, Free Sketch Space %d\n\r", ESP.getSketchSize(), ESP.getFreeSketchSpace());
    Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n\r", ESP.getPsramSize(), ESP.getFreePsram());
    Serial.printf("Chip Model %s, ChipRevision %d, Cpu Freq %d, SDK Version %s\n\r", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
    Serial.printf("Flash Size %d, Flash Speed %d\n\r", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
    Serial.println("##################################\n");
    ctx()->setup();
}

void loop() {
  ctx()->eventLoop.tick();
}