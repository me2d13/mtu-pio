#include "net.h"
#include <string>
#include <WiFi.h>
#include "config.h"
#include "Logger.h"
#include <time.h>
#include <ArduinoOTA.h>
#include <esp_system.h>

// Why did the chip come up this time? Panic, watchdog or brownout all look the
// same from the outside (a silent reboot), so this is worth recording.
std::string resetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_UNKNOWN:   return "unknown";
        case ESP_RST_POWERON:   return "power on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software restart";
        case ESP_RST_PANIC:     return "panic / crash";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT:  return "brownout (power drop)";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "reason " + std::to_string((int)esp_reset_reason());
    }
}

// NTP server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // GMT offset in seconds (e.g., +1 for UTC+1)
const int daylightOffset_sec = 0; //3600; // Daylight savings offset in seconds (adjust if needed)
bool otaUploadInProgress = false;

void setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }
}

std::string getIp() {
    return std::string(WiFi.localIP().toString().c_str());
}

std::string getTimeStr() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::string(timeStr);
    } else {
        return std::string("?");
    }
}

void syncNtp() {
    // Initialize time via NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    logger.print("Time synchronized with NTP server at milis ");
    logger.println(millis());
    // Log initial time
    logger.log("Current time: " + getTimeStr());
}

void setupOTA() {
    ArduinoOTA.setHostname("ESP32-ESPNOW-GW-OTA");
    //ArduinoOTA.setPassword("admin");
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        logger.print("Start updating ");
        logger.println(type.c_str());
        otaUploadInProgress = true;
    });
    ArduinoOTA.onEnd([]() {
        logger.println("End");
        otaUploadInProgress = false;
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        logger.print("Ota error: ");
        if (error == OTA_AUTH_ERROR) {
            logger.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            logger.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            logger.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            logger.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            logger.println("End Failed");
        }
        otaUploadInProgress = false;
    });
    ArduinoOTA.begin();
}

bool isOtaUploadInProgress() {
    return otaUploadInProgress;
}