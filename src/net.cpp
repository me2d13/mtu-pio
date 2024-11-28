#include "net.h"
#include <string>
#include <WiFi.h>
#include "config.h"
#include "Logger.h"
#include <time.h>

// NTP server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // GMT offset in seconds (e.g., +1 for UTC+1)
const int daylightOffset_sec = 0; //3600; // Daylight savings offset in seconds (adjust if needed)

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

void syncNtp() {
    // Initialize time via NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    logger.log("Time synchronized with NTP server.");

   // Log initial time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        logger.log("Current time: " + std::string(timeStr));
    } else {
        logger.log("Failed to get time.");
    }
}