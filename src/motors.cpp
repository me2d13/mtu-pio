#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "Motor.h"
#include "Logger.h"
#include "context.h"
#include "motors.h"
#include "config.h"

#define R_SENSE 0.11f

HardwareSerial motorSerial(1);
TMC2208Stepper driver(&motorSerial, R_SENSE);

void setupMotors() {
    ctx()->motorsController.setup();
}

void MotorsController::scheduleSetup(unsigned long delay) {
    motorsInitTask.set(TASK_IMMEDIATE, TASK_ONCE, setupMotors);
    ctx()->taskScheduler.addTask(motorsInitTask);
    motorsInitTask.enableDelayed(delay);
    logger.log("Motors setup planned");
}

void MotorsController::setup() {
    motorSerial.begin(115200, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    pinMode(PIN_MOTOR_ADDR_0, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_1, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_2, OUTPUT);
    logger.log("Motors UART addr bits configured");
    /*for (int i = 0; i < MOTORS_COUNT; i++) {
        motors[i].init(&driver);
    }*/
    motors[0].init(&driver);
}

void MotorsController::selectMotorUart(uint8_t addr) {
    digitalWrite(PIN_MOTOR_ADDR_0, addr & 0x01);
    digitalWrite(PIN_MOTOR_ADDR_1, addr & 0x02);
    digitalWrite(PIN_MOTOR_ADDR_2, addr & 0x04);
    logger.print("Selecting UART for motor ");
    logger.println(addr);
}

Motor *MotorsController::getMotor(int index) {
    return &motors[index];
}

void MotorsController::handleApiCommand(int index, String command, AsyncWebServerRequest *request, JsonObject jsonObj) {
    if (command.equals("enable")) {
        motors[index].enable();
        request->send(200, "text/plain", "Motor enabled");
    } else if (command.equals("disable")) {
        motors[index].disable();
        request->send(200, "text/plain", "Motor disabled");
    } else if (command.equals("runAtSpeed")) {
        int speed = jsonObj["parameters"]["speed"].as<int>();
        motors[index].turnBySpeed(speed);
        request->send(200, "text/plain", "Run at speed executed with speed " + String(speed));
    } else {
        request->send(400, "text/plain", "Unknown command " + command);
    }
}