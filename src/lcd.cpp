#include <Wire.h>
#include <LCD-I2C.h>
#include "Logger.h"
#include "net.h"
#include "esp32/clk.h"
#include "context.h"
#include "axis.h"
#include "config.h"
#include "i2c.h"

#define PRINT_ABOUT 0

LCD_I2C lcd(0x27, 20, 4);

void lcdAbout();

void setupLcd() {
    ctx()->i2c()->channel(I2C_CHANNEL_LCD);
    //delay(100);
    lcd.begin(ctx()->i2c()->peripherals());
    lcd.display();
    lcd.backlight();
    //lcd.backlightOff();

    if (PRINT_ABOUT) {
        ctx()->eventLoop.onRepeat(1000, [] () {
            if (!ctx()->i2c()->isScanning(PERIPHERALS)) {
                ctx()->i2c()->channel(I2C_CHANNEL_LCD);
                lcdAbout();
            }
        });
    }
}

void lcdAbout() {
    lcd.setCursor(0, 0);
    lcd.print("IP: ");
    lcd.print(getIp().c_str());
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
    lcd.print(getAxisValue(0));
    lcd.print("    ");
    lcd.setCursor(0, 3);
    lcd.print(getTimeStr().c_str());
}

void displaySlowClockCalibration() { uint32_t slow_clk_cal = esp_clk_slowclk_cal_get(); Serial.print("Slow Clock Calibration Value: "); Serial.print(slow_clk_cal); Serial.println(" microseconds"); }

void displayCpuFrequency() { int cpu_freq = esp_clk_cpu_freq(); Serial.print("CPU Frequency: "); Serial.print(cpu_freq); Serial.println(" Hz"); }

void displayApbFrequency() { int apb_freq = esp_clk_apb_freq(); Serial.print("APB Frequency: "); Serial.print(apb_freq); Serial.println(" Hz"); }
