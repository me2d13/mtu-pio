#include <Wire.h>
#include <LCD-I2C.h>
#include "Logger.h"
#include "net.h"
#include "esp32/clk.h"
#include "context.h"
#include "axis.h"
#include "config.h"
#include "i2c.h"
#include <TaskSchedulerDeclarations.h>

#define PRINT_ABOUT 1

#define fullDEBUG(m) logger.log(m)
#define serialDEBUG(m) Serial.println(m)
#define DEBUG(m) // Serial.println(m)

LCD_I2C lcd(0x27, 20, 4);

Task lcdTask;

void lcdAbout();

void setupLcd() {
    DEBUG("Setting up LCD, selecting channel");
    ctx()->i2c()->channel(I2C_CHANNEL_LCD);
    DEBUG("Channel seleced, initializing LCD");
    //delay(100);
    lcd.begin(ctx()->i2c()->peripherals());
    DEBUG("LCD initialized");
    lcd.display();
    DEBUG("LCD displayed");
    lcd.backlight();
    DEBUG("LCD backlight on");
    //lcd.backlightOff();

    if (PRINT_ABOUT) {
        lcdTask.setInterval(1000);
        lcdTask.setIterations(TASK_FOREVER);
        lcdTask.setCallback([]() {
            if (!ctx()->i2c()->isScanning(PERIPHERALS)) {
                ctx()->i2c()->channel(I2C_CHANNEL_LCD);
                lcdAbout();
            }
        });
        ctx()->taskScheduler.addTask(lcdTask);
        lcdTask.enable();
        lcd.clear();
        DEBUG("LCD clear done");
        lcdAbout();
        DEBUG("LCD about printed");
    }
}

void lcdAbout() {
    //Serial.print("CPU frequency: ");
    //Serial.println(esp_clk_cpu_freq());
    lcd.setCursor(0, 0);
    DEBUG("Set cursor done");
    lcd.print("IP: ");
    DEBUG("Print IP static text done");
    lcd.print(getIp().c_str());
    DEBUG("Printing IP done");
    lcd.setCursor(7, 1); // Or setting the cursor in the desired position.
    lcd.print("M T U");
    if (1==0) {
        int cpu_freq = esp_clk_cpu_freq();
        int apb_freq = esp_clk_apb_freq();
        lcd.setCursor(0, 2);
        lcd.print(cpu_freq);
        lcd.print(" / ");
        lcd.print(apb_freq);
    }
    lcd.setCursor(0, 2);
    lcd.print("X:");
    lcd.print(ctx()->axesController.getAxisValue(0));
    lcd.print("    ");
    DEBUG("Printing X done");
    lcd.setCursor(0, 3);
    lcd.print(getTimeStr().c_str());
    DEBUG("Printing time done");
}

void displaySlowClockCalibration() { uint32_t slow_clk_cal = esp_clk_slowclk_cal_get(); Serial.print("Slow Clock Calibration Value: "); Serial.print(slow_clk_cal); Serial.println(" microseconds"); }

void displayCpuFrequency() { int cpu_freq = esp_clk_cpu_freq(); Serial.print("CPU Frequency: "); Serial.print(cpu_freq); Serial.println(" Hz"); }

void displayApbFrequency() { int apb_freq = esp_clk_apb_freq(); Serial.print("APB Frequency: "); Serial.print(apb_freq); Serial.println(" Hz"); }
