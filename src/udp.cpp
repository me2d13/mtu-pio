#include <Arduino.h>
#include "udp.h"
#include "context.h"
#include "Logger.h"
#include "state.h"
#include "config.h"

#include <WiFiUdp.h>

WiFiUDP udp;
#define UDP_PORT 49152
#define UDP_BUFFER_SIZE 255
#define UDP_CHECK_INTERVAL 100
#define MAX_ERROR_LOG_COUNT 10
#define MIN_PACKET_SIZE 41

// Capture xpl UDP data: ncat -ul 49152 > xpldata.out

void XplaneInterface::setup()
{
    udp.begin(UDP_PORT);
    logger.print("UDP server started on port ");
    logger.println(UDP_PORT);
    udpCheckTask.set(UDP_CHECK_INTERVAL, TASK_FOREVER, [&]() { loopUdp(); });
    ctx()->taskScheduler.addTask(udpCheckTask);
    udpCheckTask.enable();
}

void XplaneInterface::parsePacket(char *buffer, int len)
{
    // packet format description: https://questions.x-plane.com/20760/where-can-i-get-x-plane-11-complete-udp-protocol
    if (len < MIN_PACKET_SIZE) {
        sprintf(errorMessage, "UDP packet too short: %d bytes, expected at least %d", len, MIN_PACKET_SIZE);
        logError();
        return;
    }
    if (strncmp(buffer, "DATA", 4) != 0) {
        sprintf(errorMessage, "Invalid UDP packet, expected DATA at start");
        logError();
        return;
    }
    int pos = 5;
    // after data we have 1..n groups of:
    //   1 byte message type (0x19 or 0x20)
    //   3 bytes of padding (0x00)
    //   8 floats each 4 bytes
    while (pos < len) {
        char messageType = buffer[pos];
        // 25 commanded throttle (0x19)
        // 26 actual throttle (0x1A)
        // 13 trims (0x0D)
        if (messageType != 0x19 && messageType != 0x1A && messageType != 0x0D) {
            sprintf(errorMessage, "Unsupported data type %d. Expected message type 13, 25 or 26. Skipping.", messageType);
            logError();
            pos += 1 + 3 + 8 * 4; // 1 byte message type + 3 bytes padding + 8 floats
        } else {
            pos += 1 + 3; // 1 byte message type + 3 bytes padding
            float *data = (float *)(buffer + pos);
            char message[200];
            sprintf(message, "UDP packet received type %d, data %f, %f ...", messageType, data[0], data[1]);
            logger.log(message);
            pos += 8 * 4; // 8 floats of 4 bytes each
        }
    }
}

void XplaneInterface::loopUdp()
{
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(packetBuffer, UDP_BUFFER_SIZE - 1);
        if (len > 0) {
            packetBuffer[len] = 0;
            parsePacket(packetBuffer, len);
        }
    }
}

void XplaneInterface::logError()
{
    errorsLogCount++;
    if (errorsLogCount < MAX_ERROR_LOG_COUNT) {
        String message = "Error in UDP message received: " + String(packetBuffer);
        logger.log(message.c_str());
        logger.log(errorMessage);
    } else if (errorsLogCount == MAX_ERROR_LOG_COUNT) {
        logger.log("Too many xplane interface errors, skipping further logging.");
    }
}