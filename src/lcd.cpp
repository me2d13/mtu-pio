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

#define ENABLE_LCD_PROFILING 0

#define COLS 20
#define ROWS 4

LCD_I2C lcd(0x27, COLS, ROWS);

Task lcdTask;

void lcdAbout();

char lcdState[COLS * ROWS + 1] = {0};
char lcdCanvas[COLS * ROWS + 1] = {0};

// compare canvas with lcd state and update lcd if needed
// do minimal updates to lcd - only when byte in state is different from canvas
int printCanvasToLcd() {
    auto start = millis();
    int charsUpdated = 0;
    bool setCursorNeeded = true;
    for (int row = 0; row < ROWS; row++) {
        setCursorNeeded = true;
        for (int col = 0; col < COLS; col++) {
            int i = row * COLS + col;
            if (lcdState[i] != lcdCanvas[i]) {
                if (setCursorNeeded) {
                    lcd.setCursor(col, row);
                    // if we update more than 1 char, we don't need to set cursor again
                    setCursorNeeded = false;
                }
                lcd.write(lcdCanvas[i]);
                lcdState[i] = lcdCanvas[i];
                charsUpdated++;
            } else {
                // we skipped char, need to set cursor again
                setCursorNeeded = true;
            }
        }
    }
    if (ENABLE_LCD_PROFILING) {
        auto end = millis();
        char message[100];
        sprintf(message, "lcd update took %d ms, chars updated: %d", end - start, charsUpdated);
        logger.log(message);
        // full screen: lcd update took 91 ms, chars updated: 80
        // seconds update: lcd update took 4 ms, chars updated: 2
        // axis value update: lcd update took 11 ms, chars updated: 7
    }
    return charsUpdated;
}

void printTestPage(char *canvas) {
    strcpy(canvas, 
           "line1:78901234567890"
           "line2: hijklmnopqrst"
           "line3:--------------"
           "line4: hijklmnopqrst"
    );
}

void lcdTest() {
    printTestPage(lcdCanvas);
    printCanvasToLcd();
}


void setupLcd() {
    if (ENABLE_LCD == 0) {
        logger.log("Lcs setup skipped, ENABLE_LCD is 0");
        return;
    }
    ctx()->i2c()->channel(I2C_CHANNEL_LCD);
    lcd.begin(ctx()->i2c()->peripherals());
    lcd.display();
    lcd.backlight();
    //lcd.backlightOff();

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
    lcdAbout();
    //lcdTest();
}

void printToCanvas(int col, int row, const char *text) {
    // print text to canvs buffer without null terminator
    // if text is too long, truncate it to fit in the buffer
    int len = strlen(text);
    if (len > COLS - col) {
        len = COLS - col;
    }
    for (int i = 0; i < len; i++) {
        lcdCanvas[row * COLS + col + i] = text[i];
    }
}

void printToCanvasRpad(int col, int row, int value, int width) {
    // print value to canvas buffer with right padding
    // if value is too long, truncate it to fit in the buffer
    char text[10];
    sprintf(text, "%d", value);
    int len = strlen(text);
    if (len > width) {
        len = width;
    }
    for (int i = 0; i < len; i++) {
        lcdCanvas[row * COLS + col + i] = text[i];
    }
    for (int i = len; i < width; i++) {
        lcdCanvas[row * COLS + col + i] = ' ';
    }
}

void lcdAbout() {
    memccpy(lcdCanvas, 
        "IP :                "
        "      M  T  U       "
        "T1: ????   T2: ???? "
        "                    "
        , 0, COLS * ROWS);
    printToCanvas(4, 0, getIp().c_str());
    printToCanvasRpad(4, 2, ctx()->state.transient.getAxisValue(1), 4);
    printToCanvasRpad(15, 2, ctx()->state.transient.getAxisValue(2), 4);
    printToCanvas(0, 3, getTimeStr().c_str());
    printCanvasToLcd();
}