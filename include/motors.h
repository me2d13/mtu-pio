#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include "Motor.h"
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>

#define MOTORS_COUNT 6
#define MOTOR_THR1 0
#define MOTOR_THR2 1
#define MOTOR_SPEED_BRAKE 2
#define MOTOR_TRIM 3
#define MOTOR_TRIM_IND_1 4
#define MOTOR_TRIM_IND_2 5

class MotorsController
{
private:
    Task motorsInitTask;
    Motor motors[MOTORS_COUNT] = {
        Motor(MOTOR_THR1, 42, 41),
        Motor(MOTOR_THR2, 40, 39),
        Motor(MOTOR_SPEED_BRAKE, 38, 37),
        Motor(MOTOR_TRIM, 36, 35),
        Motor(MOTOR_TRIM_IND_1, 48, 47),
        Motor(MOTOR_TRIM_IND_2, 21, 20)
    };
public:
    void setup();
    void scheduleSetup(unsigned long delay);
    void selectMotorUart(uint8_t addr);
    Motor *getMotor(int index);
    void handleApiCommand(int index, String command, AsyncWebServerRequest *request, JsonObject jsonObj);
};
