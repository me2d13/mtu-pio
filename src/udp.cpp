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
#define MSFS_THROTTLE_1 "THR1"
#define MSFS_THROTTLE_2 "THR2"
#define MSFS_PARKING_BRAKE "PARK_BRAKE"
#define MSFS_AT_STATE "AT_ARM"
#define MSFS_SPEED_BRAKE "SPEED_BRAKE"
#define MSFS_TRIM "TRIM"

#define _UDP_PROFILING
#define _UDP_LOGGING
#define UDP_PROFILING_BUFFER_SIZE 20

// Capture simulator UDP data: ncat -ul 49152 > simdata.out

// Send UDP packet - X-Plane format:
// echo -n '{"sim/flightmodel/engine/ENGN_thro": [0.2, 0.0], "laminar/B738/autopilot/autothrottle_status1": 1.0'} | ncat -4u mtu.local 49152
// echo -n '{"laminar/B738/flight_model/stab_trim_units": 5.0}' | ncat -4u 192.168.1.112 49152
// echo -n '{"laminar/B738/flt_ctrls/speedbrake_lever": 0.5}' | ncat -4u mtu.local 49152

// Send UDP packet - MSFS format:
// echo -n '{"GENERAL ENG THROTTLE LEVER POSITION:1": 0.2, "GENERAL ENG THROTTLE LEVER POSITION:2": 0.1, "AUTOPILOT THROTTLE ARM": 1.0}' | ncat -4u mtu.local 49152
// echo -n '{"GENERAL ENG THROTTLE LEVER POSITION:1": 0.0, "BRAKE PARKING INDICATOR": 0.0}' | ncat -4u mtu.local 49152
// echo -n '{"GENERAL ENG THROTTLE LEVER POSITION:1": 0.0, "SPOILERS HANDLE POSITION": 0.5, "ELEVATOR TRIM INDICATOR": 0.3}' | ncat -4u mtu.local 49152


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
#ifdef UDP_LOGGING
    logger.println("Udp packet arrived");
#endif
    
    
    applyPayload(doc.as<JsonObject>());
}

// Shared entry point for UDP packets and for values posted to /api/sim.
bool SimUdpInterface::applyPayload(JsonObject data)
{
    // Backup current sim data so we can detect if it was updated
    sim_data backupSimData = *ctx()->state.transient.getSimData();
    
    // Detect simulator type based on presence of keys and parse accordingly
    // Check for X-Plane specific key
    if (data.containsKey(DATAREF_THROTTLE)) {
#ifdef UDP_LOGGING
        logger.println("Udp packet recognized as X-plane");
#endif
        parseXplaneData(data);
    }
    // Check for MSFS specific key
    else if (data.containsKey(MSFS_THROTTLE_1)) {
#ifdef UDP_LOGGING
        logger.println("MSFS packet recognized as X-plane");
#endif
        parseMsfsData(data);
    }
    else {
        sprintf(errorMessage, "Unknown simulator data format - no recognized keys found");
        logError();
        return false;
    }

    commitChange(backupSimData);
    return true;
}

// Friendly keys used by the Simulator page mock: throttle1, throttle2, speedBrake, trim,
// parkingBrake, autoThrottle. A body without any of them is treated as a raw simulator payload,
// so the endpoint also accepts exactly what the UDP port accepts.
String SimUdpInterface::applyMock(JsonObject data)
{
    bool hasMockKey = data.containsKey("throttle1") || data.containsKey("throttle2") ||
                      data.containsKey("speedBrake") || data.containsKey("trim") ||
                      data.containsKey("parkingBrake") || data.containsKey("autoThrottle");

    if (!hasMockKey) {
        return applyPayload(data) ? "" : "No known key in the body - expected throttle1, throttle2, speedBrake, trim, parkingBrake or autoThrottle";
    }

    sim_data *sim = ctx()->state.transient.getSimData();
    sim_data backupSimData = *sim;
    sim_data next = *sim;

    JsonVariant value = data["throttle1"];
    if (!value.isNull()) {
        if (!value.is<float>() && !value.is<int>()) return "throttle1 must be a number";
        float parsed = value.as<float>();
        if (parsed < 0.0f || parsed > 1.0f) return "throttle1 must be between 0 and 1 (0 = idle, 1 = full thrust)";
        next.throttle1 = parsed;
    }

    value = data["throttle2"];
    if (!value.isNull()) {
        if (!value.is<float>() && !value.is<int>()) return "throttle2 must be a number";
        float parsed = value.as<float>();
        if (parsed < 0.0f || parsed > 1.0f) return "throttle2 must be between 0 and 1 (0 = idle, 1 = full thrust)";
        next.throttle2 = parsed;
    }

    value = data["speedBrake"];
    if (!value.isNull()) {
        if (!value.is<float>() && !value.is<int>()) return "speedBrake must be a number";
        float parsed = value.as<float>();
        if (parsed < 0.0f || parsed > 1.0f) return "speedBrake must be between 0 and 1 (0 = retracted, 1 = full)";
        next.speedBrake = parsed;
    }

    value = data["trim"];
    if (!value.isNull()) {
        if (!value.is<float>() && !value.is<int>()) return "trim must be a number";
        float parsed = value.as<float>();
        if (parsed < -20.0f || parsed > 20.0f) return "trim must be between -20 and 20 trim units";
        next.trim = parsed;
    }

    value = data["parkingBrake"];
    if (!value.isNull()) {
        if (value.is<bool>()) {
            next.parkingBrake = value.as<bool>();
        } else if (value.is<float>() || value.is<int>()) {
            float parsed = value.as<float>();
            if (parsed < 0.0f || parsed > 1.0f) return "parkingBrake must be true/false or 0/1";
            next.parkingBrake = parsed > 0.5f;
        } else {
            return "parkingBrake must be true/false or 0/1";
        }
    }

    value = data["autoThrottle"];
    if (!value.isNull()) {
        if (value.is<bool>()) {
            next.autoThrottle = value.as<bool>();
        } else if (value.is<float>() || value.is<int>()) {
            float parsed = value.as<float>();
            if (parsed < 0.0f || parsed > 1.0f) return "autoThrottle must be true/false or 0/1";
            next.autoThrottle = parsed > 0.5f;
        } else {
            return "autoThrottle must be true/false or 0/1";
        }
    }

    // Everything checked, only then write it in - same struct the UDP parser fills.
    *sim = next;

    char message[160];
    sprintf(message, "Mock simulator values applied: throttle1=%.2f throttle2=%.2f speedBrake=%.2f trim=%.2f parkingBrake=%s autoThrottle=%s",
            sim->throttle1, sim->throttle2, sim->speedBrake, sim->trim,
            sim->parkingBrake ? "on" : "off", sim->autoThrottle ? "on" : "off");
    logger.log(message);

    commitChange(backupSimData);
    return "";
}

// Timestamps the update and hands it to the motor drivers, exactly like an arriving packet.
void SimUdpInterface::commitChange(sim_data backup)
{
    sim_data *sim = ctx()->state.transient.getSimData();

    // Update timestamp
    sim->lastUpdateTime = millis();
    
    // Check if we have new data and update the state
    if (memcmp(&backup, sim, sizeof(sim_data)) != 0) {
#ifdef UDP_LOGGING
        logger.println("State data modified by UDP, calling change handler");
#endif
        // Data changed, update the state
        ctx()->simDataDriver.simDataChanged(backup, *sim);
    }
}

void SimUdpInterface::parseXplaneData(JsonObject data)
{
    // Parse X-Plane specific data format
    ctx()->state.transient.getSimData()->throttle1 = data[DATAREF_THROTTLE][0] | ctx()->state.transient.getSimData()->throttle1;
    ctx()->state.transient.getSimData()->throttle2 = data[DATAREF_THROTTLE][1] | ctx()->state.transient.getSimData()->throttle2;
    ctx()->state.transient.getSimData()->trim = data[DATAREF_TRIM] | ctx()->state.transient.getSimData()->trim;
    
    float parkingBrakeValue = data[DATAREF_PARKING_BRAKE] | -1.0f;
    if (parkingBrakeValue >= 0.0f) {
        ctx()->state.transient.getSimData()->parkingBrake = parkingBrakeValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    float autoThrottleValue = data[DATAREF_AT_STATE] | -1.0f;
    if (autoThrottleValue >= 0.0f) {
        ctx()->state.transient.getSimData()->autoThrottle = autoThrottleValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    ctx()->state.transient.getSimData()->speedBrake = data[DATAREF_SPEED_BRAKE] | ctx()->state.transient.getSimData()->speedBrake;
}

void SimUdpInterface::parseMsfsData(JsonObject data)
{
    // Parse MSFS specific data format
    // MSFS provides throttle as two separate variables (not an array)
    ctx()->state.transient.getSimData()->throttle1 = data[MSFS_THROTTLE_1] | ctx()->state.transient.getSimData()->throttle1;
    ctx()->state.transient.getSimData()->throttle2 = data[MSFS_THROTTLE_2] | ctx()->state.transient.getSimData()->throttle2;
    float msfsTrim = data[MSFS_TRIM] | 100000.0;
    if (msfsTrim < 90000) {
        ctx()->state.transient.getSimData()->trim = convertMsfsTrim(msfsTrim);
    }
    
    float parkingBrakeValue = data[MSFS_PARKING_BRAKE] | -1.0f;;
    if (parkingBrakeValue >= 0.0f) {
        ctx()->state.transient.getSimData()->parkingBrake = parkingBrakeValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    float autoThrottleValue = data[MSFS_AT_STATE] | -1.0f;
    if (autoThrottleValue >= 0.0f) {
        ctx()->state.transient.getSimData()->autoThrottle = autoThrottleValue > 0.5f; // 0.0 = off, 1.0 = on
    }
    
    ctx()->state.transient.getSimData()->speedBrake = data[MSFS_SPEED_BRAKE] | ctx()->state.transient.getSimData()->speedBrake;
}

float SimUdpInterface::convertMsfsTrim(float simValue) {
    // simValue comes as float between -16k and 16k.
    // We need to convert it to indicator output between 0 and 17
    // for output 0 the sim value is -16384 in unit "position 16k" or -0.6450588 in "position"
    // for output 17 the sim value is 16384 in unit "position 16k" or 1 in "position"
    constexpr float inMin = -0.6450588f;
    constexpr float inMax =  1.0f;
    constexpr float outMin = 0.0f;
    constexpr float outMax = 17.0f;

    if (simValue <= inMin) return outMin;
    if (simValue >= inMax) return outMax;

    float ratio = (simValue - inMin) / (inMax - inMin);
    return outMin + ratio * (outMax - outMin);
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