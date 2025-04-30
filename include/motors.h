#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include "config.h"
#include "Motor.h"
#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>

class MotorsController
{
private:
    Task motorsInitTask;
    Motor motors[MOTORS_COUNT] = {
        Motor(MOTOR_THR1, 42, 41),
        Motor(MOTOR_THR2, 40, 39),
/*      
By wrong HW design pins 35-37 are reserved for PSRAM, so using them as DIR/STEP is not possible
When setting their mode to OUTPUT, the ESP32 will crash.
Luckily I don't need exact stepping for those 2 motors, I can survive with
running them at speed with UART command.
So linking their pins to THR2 as we don't use stepping for THR2 anyway.

Source: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide.html

"For boards with Octal SPI flash/PSRAM memory embedded ESP32-S3-WROOM-1/1U modules, and boards with ESP32-S3-WROOM-2 modules, 
the pins GPIO35, GPIO36 and GPIO37 are used for the internal communication between ESP32-S3 and SPI flash/PSRAM memory, 
thus not available for external use."

Also 20 is out, source: https://github.com/atomic14/esp32-s3-pinouts

        Motor(MOTOR_SPEED_BRAKE, 38, 37),
        Motor(MOTOR_TRIM, 36, 35),*/
        Motor(MOTOR_SPEED_BRAKE, 40, 39),
        Motor(MOTOR_TRIM, 40, 39),
        Motor(MOTOR_TRIM_IND_1, 48, 47),
        Motor(MOTOR_TRIM_IND_2, 21, 39) //20)
    };
public:
    void setup();
    void scheduleSetup(unsigned long delay);
    void selectMotorUart(uint8_t addr);
    Motor *getMotor(int index);
    void handleApiCommand(int index, String command, AsyncWebServerRequest *request, JsonObject jsonObj);
};
