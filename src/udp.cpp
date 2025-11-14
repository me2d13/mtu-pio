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

// X-Plane datarefs
#define DATAREF_THROTTLE "sim/flightmodel/engine/ENGN_thro"
//define DATAREF_TRIM "sim/flightmodel2/controls/elevator_trim"
#define DATAREF_TRIM "laminar/B738/flight_model/stab_trim_units"
#define DATAREF_PARKING_BRAKE "laminar/B738/parking_brake_pos"
//#define DATAREF_SPEED_MODE "laminar/B738/autopilot/speed_mode"
#define DATAREF_SPEED_BRAKE "laminar/B738/flt_ctrls/speedbrake_lever"
#define DATAREF_AT_STATE "laminar/B738/autopilot/autothrottle_status1"

// MSFS SimConnect variables (placeholders - to be updated)
#define MSFS_THROTTLE_1 "GENERAL_ENG_THROTTLE_LEVER_POSITION_1"
#define MSFS_THROTTLE_2 "GENERAL_ENG_THROTTLE_LEVER_POSITION_2"
#define MSFS_PARKING_BRAKE "BRAKE_PARKING_POSITION"
#define MSFS_AT_STATE "AUTOPILOT_THROTTLE_ARM"
#define MSFS_SPEED_BRAKE "SPOILERS_HANDLE_POSITION"
#define MSFS_TRIM "ELEVATOR_TRIM_POSITION"

#define _UDP_PROFILING
#define UDP_PROFILING_BUFFER_SIZE 20

// Capture simulator UDP data: ncat -ul 49152 > simdata.out

// Send UDP packet - X-Plane format:
// echo -n '{"sim/flightmodel/engine/ENGN_thro": [0.2, 0.0]}' | ncat -4u 192.168.1.112 49152
// echo -n '{"laminar/B738/flight_model/stab_trim_units": 5.0}' | ncat -4u 192.168.1.112 49152
// echo -n '{"laminar/B738/flt_ctrls/speedbrake_lever": 0.5}' | ncat -4u 192.168.1.112 49152

// Send UDP packet - MSFS format:
// echo -n '{"GENERAL_ENG_THROTTLE_LEVER_POSITION_1": 0.2, "GENERAL_ENG_THROTTLE_LEVER_POSITION_2": 0.0}' | ncat -4u 192.168.1.112 49152
// echo -n '{"BRAKE_PARKING_POSITION": 1.0, "AUTOPILOT_THROTTLE_ARM": 1.0}' | ncat -4u 192.168.1.112 49152
// echo -n '{"SPOILERS_HANDLE_POSITION": 0.5, "ELEVATOR_TRIM_POSITION": 0.3}' | ncat -4u 192.168.1.112 49152

// from https://developer.x-plane.com/article/x-plane-web-api/
//  curl 'http://localhost:8086/api/v2/datarefs?filter\[name\]=laminar/B738/autopilot/speed_mode' -H 'Accept: application/json, text/plain, */*'
//  curl 'http://localhost:8086/api/v2/datarefs/2463703123728/value'   -H 'Accept: application/json, text/plain, */*'

#ifdef UDP_PROFILING
unsigned long lastTaskStart = 0;
int taskRunDelays[UDP_PROFILING_BUFFER_SIZE];
int taskRunDelayIndex = 0;
#endif

void SimUdpInterface::setup()
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

void SimUdpInterface::parsePacket(char *buffer, int len)
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
    
    // Backup current sim data so we can detect if it was updated
    sim_data backupSimData = *ctx()->state.transient.getSimData();
    
    // Detect simulator type based on presence of keys and parse accordingly
    // Check for X-Plane specific key
    if (doc.containsKey(DATAREF_THROTTLE)) {
        parseXplaneData(doc);
    }
    // Check for MSFS specific key
    else if (doc.containsKey(MSFS_THROTTLE_1)) {
        parseMsfsData(doc);
    }
    else {
        sprintf(errorMessage, "Unknown simulator data format - no recognized keys found");
        logError();
        return;
    }
    
    // Update timestamp
    ctx()->state.transient.getSimData()->lastUpdateTime = millis();
    
    // Check if we have new data and update the state
    if (memcmp(&backupSimData, ctx()->state.transient.getSimData(), sizeof(sim_data)) != 0) {
        // Data changed, update the state
        ctx()->simDataDriver.simDataChanged(backupSimData, *ctx()->state.transient.getSimData());
    }
}

void SimUdpInterface::parseXplaneData(JsonDocument &doc)
{
    // Parse X-Plane specific data format
    ctx()->state.transient.getSimData()->throttle1 = doc[DATAREF_THROTTLE][0] | ctx()->state.transient.getSimData()->throttle1;
    ctx()->state.transient.getSimData()->throttle2 = doc[DATAREF_THROTTLE][1] | ctx()->state.transient.getSimData()->throttle2;
    ctx()->state.transient.getSimData()->trim = doc[DATAREF_TRIM] | ctx()->state.transient.getSimData()->trim;
    
    float parkingBrakeValue = doc[DATAREF_PARKING_BRAKE] | -1.0f;
    if (parkingBrakeValue >= 0.0f) {
        ctx()->state.transient.getSimData()->parkingBrake = parkingBrakeValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    float autoThrottleValue = doc[DATAREF_AT_STATE] | -1.0f;
    if (autoThrottleValue >= 0.0f) {
        ctx()->state.transient.getSimData()->autoThrottle = autoThrottleValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    ctx()->state.transient.getSimData()->speedBrake = doc[DATAREF_SPEED_BRAKE] | ctx()->state.transient.getSimData()->speedBrake;
}

void SimUdpInterface::parseMsfsData(JsonDocument &doc)
{
    // Parse MSFS specific data format
    // MSFS provides throttle as two separate variables (not an array)
    ctx()->state.transient.getSimData()->throttle1 = doc[MSFS_THROTTLE_1] | ctx()->state.transient.getSimData()->throttle1;
    ctx()->state.transient.getSimData()->throttle2 = doc[MSFS_THROTTLE_2] | ctx()->state.transient.getSimData()->throttle2;
    ctx()->state.transient.getSimData()->trim = doc[MSFS_TRIM] | ctx()->state.transient.getSimData()->trim;
    
    float parkingBrakeValue = doc[MSFS_PARKING_BRAKE] | -1.0f;
    if (parkingBrakeValue >= 0.0f) {
        ctx()->state.transient.getSimData()->parkingBrake = parkingBrakeValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    float autoThrottleValue = doc[MSFS_AT_STATE] | -1.0f;
    if (autoThrottleValue >= 0.0f) {
        ctx()->state.transient.getSimData()->autoThrottle = autoThrottleValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    ctx()->state.transient.getSimData()->speedBrake = doc[MSFS_SPEED_BRAKE] | ctx()->state.transient.getSimData()->speedBrake;
}

void SimUdpInterface::loopUdp()
{
    #ifdef UDP_PROFILING
      // verify & log correct timing of UDP check task
      auto now = millis();
      unsigned long fromLastRun = now - lastTaskStart;
      lastTaskStart = now;
      taskRunDelays[taskRunDelayIndex++] = fromLastRun;
      if (taskRunDelayIndex == UDP_PROFILING_BUFFER_SIZE) {
        logger.print("UDP check timing delays:");
        for (int i = 0; i < UDP_PROFILING_BUFFER_SIZE; i++) {
            logger.print(" ");
            logger.print(taskRunDelays[i]);
        }
        logger.println(".");
        taskRunDelayIndex = 0;
      }
    #endif
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

void SimUdpInterface::logError()
{
    errorsLogCount++;
    if (errorsLogCount < MAX_ERROR_LOG_COUNT) {
        logger.log("Error in UDP message");
        logger.log(errorMessage);
    } else if (errorsLogCount == MAX_ERROR_LOG_COUNT) {
        logger.log("Too many simulator UDP interface errors, skipping further logging.");
    }
}