#include <Wire.h>
#include <LCD-I2C.h>
#include "Logger.h"
#include "net.h"

LCD_I2C lcd(0x27, 20, 4);

void setupLcd(TwoWire& wire) {
    lcd.begin(&wire);
    lcd.display();
    lcd.backlight();
}

void lcdAbout() {
    lcd.setCursor(0, 0);
    lcd.print("IP: ");
    lcd.print(getIp().c_str());
    lcd.setCursor(7, 1); // Or setting the cursor in the desired position.
    lcd.print("M T U");
    lcd.setCursor(0, 3);
    lcd.print(getTimeStr().c_str());
}