#include <Arduino.h>
#include "udp.h"
#include "context.h"
#include "Logger.h"
#include "state.h"
#include "config.h"
#include <ArduinoJson.h>

#include <WiFiUdp.h>

WiFiUDP udp;
#define UDP_PORT 49152
#define UDP_BUFFER_SIZE 255
#define UDP_CHECK_INTERVAL 100
#define MAX_ERROR_LOG_COUNT 10
#define MIN_PACKET_SIZE 41

#define MESSAGE_LOGGING 0

#define DATAREF_THROTTLE "sim/flightmodel/engine/ENGN_thro"
#define DATAREF_TRIM "sim/flightmodel2/controls/elevator_trim"
#define DATAREF_PARKING_BRAKE "laminar/B738/parking_brake_pos"
#define DATAREF_SPEED_MODE "laminar/B738/autopilot/speed_mode"


// Capture xpl UDP data: ncat -ul 49152 > xpldata.out

// from https://developer.x-plane.com/article/x-plane-web-api/
//  curl 'http://localhost:8086/api/v2/datarefs?filter\[name\]=laminar/B738/autopilot/speed_mode' -H 'Accept: application/json, text/plain, */*'
//  curl 'http://localhost:8086/api/v2/datarefs/2463703123728/value'   -H 'Accept: application/json, text/plain, */*'


void XplaneInterface::setup()
{
    if (udp.begin(UDP_PORT)) {
        logger.print("UDP server started on port ");
        logger.println(UDP_PORT);
    } else {
        logger.print("Failed to start UDP server on port ");
        logger.println(UDP_PORT);
    }
    udpCheckTask.set(UDP_CHECK_INTERVAL, TASK_FOREVER, [&]() { loopUdp(); });
    ctx()->taskScheduler.addTask(udpCheckTask);
    udpCheckTask.enable();
}

void XplaneInterface::parsePacket(char *buffer, int len)
{
    if (len == 0) {
        sprintf(errorMessage, "Empty JSON string, nothing loaded");
        logError();
        return;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer);
    if (error) {
        sprintf(errorMessage, "Failed to parse JSON: %s", error.c_str());
        logError();
        return;
    }
    // backup current xpl data so we can detect if it was updated
    xpl_data backupXplData = *ctx()->state.transient.getXplData();
    ctx()->state.transient.getXplData()->throttle1 = doc[DATAREF_THROTTLE][0] | ctx()->state.transient.getXplData()->throttle1;
    ctx()->state.transient.getXplData()->throttle2 = doc[DATAREF_THROTTLE][1] | ctx()->state.transient.getXplData()->throttle2;
    ctx()->state.transient.getXplData()->trim = doc[DATAREF_TRIM] | ctx()->state.transient.getXplData()->trim;
    float parkingBrakeValue = doc[DATAREF_PARKING_BRAKE] | -1.0f;
    if (parkingBrakeValue >= 0.0f) {
        ctx()->state.transient.getXplData()->parkingBrake = parkingBrakeValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    ctx()->state.transient.getXplData()->speedMode = doc[DATAREF_SPEED_MODE] | ctx()->state.transient.getXplData()->speedMode;
    ctx()->state.transient.getXplData()->lastUpdateTime = millis();
    // check if we have new data and update the state
    if (memcmp(&backupXplData, ctx()->state.transient.getXplData(), sizeof(xpl_data)) != 0) {
        // data changed, update the state
        if (backupXplData.parkingBrake != ctx()->state.transient.getXplData()->parkingBrake) {
            ctx()->pins.setParkingBrakeIndicator(ctx()->state.transient.getXplData()->parkingBrake);
        }
    }
}

void XplaneInterface::loopUdp()
{
    int packetSize = udp.parsePacket();
    if (packetSize) {
        char *buffer = (char *)malloc(packetSize + 1);
        if (buffer) {
            int len = udp.read(buffer, packetSize);
            if (len > 0) {
                buffer[len] = 0;
                if (MESSAGE_LOGGING) {
                    logger.print("Received UDP packet: ");
                    logger.println(buffer);
                }
                parsePacket(buffer, len);
            }
            free(buffer);
        } else {
            logger.log("Failed to allocate memory for UDP packet.");
        }
    }
}

void XplaneInterface::logError()
{
    errorsLogCount++;
    if (errorsLogCount < MAX_ERROR_LOG_COUNT) {
        logger.log("Error in UDP message");
        logger.log(errorMessage);
    } else if (errorsLogCount == MAX_ERROR_LOG_COUNT) {
        logger.log("Too many xplane interface errors, skipping further logging.");
    }
}