#include "motor.h"
#include "config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "Logger.h"

HardwareSerial motorSerial(1);

#define R_SENSE 0.11f

// temporarly
#define STEP_PIN 17
#define DIR_PIN 18

TMC2208Stepper driver(&motorSerial, R_SENSE);

void setupMotor() {
    pinMode(PIN_MOTOR_ADDR_0, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_1, OUTPUT);
    pinMode(PIN_MOTOR_ADDR_2, OUTPUT);
    selectMotor(0);
    motorSerial.begin(115200, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);


    driver.begin();
    driver.toff(5);                 // Enables driver in software
    driver.rms_current(500);        // Set motor RMS current
    driver.microsteps(4);          // Set microsteps to 1/8th

    //driver.en_pwm_mode(true);       // Toggle stealthChop on TMC2130/2160/5130/5160
    //driver.en_spreadCycle(false);   // Toggle spreadCycle on TMC2208/2209/2224
    driver.pwm_autoscale(true);     // Needed for stealthChop
    // read register
    uint8_t  version    = driver.version();
    uint16_t microsteps = driver.microsteps();
    logger.log("Motor driver version: " + std::to_string(version));
    logger.log("Microsteps: " + std::to_string(microsteps));
    driver.VACTUAL(0);
    driver.freewheel();
}

void selectMotor(uint8_t addr) {
    digitalWrite(PIN_MOTOR_ADDR_0, addr & 1);
    digitalWrite(PIN_MOTOR_ADDR_1, (addr & 2) >> 1);
    digitalWrite(PIN_MOTOR_ADDR_2, (addr & 4) >> 2);
}