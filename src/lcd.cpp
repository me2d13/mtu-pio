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
#include "Screen.h"
#include "screen/MenuScreen.h"

#define ENABLE_LCD_PROFILING 0

LCD_I2C lcd(0x27, COLS, ROWS);

Task lcdRefreshTask;

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


void ScreenController::setup() {
    hwSetup();
    lcdRefreshTask.setIterations(TASK_FOREVER);
    lcdRefreshTask.setCallback([&]() {
        if (!ctx()->i2c()->isScanning(PERIPHERALS)) {
            render();
        }
    });
    ctx()->taskScheduler.addTask(lcdRefreshTask);
    pushScreen(new MenuScreen(0));
}

void ScreenController::hwSetup() {
    if (ENABLE_LCD == 0) {
        logger.log("Lcs setup skipped, ENABLE_LCD is 0");
        return;
    }
    ctx()->i2c()->channel(I2C_CHANNEL_LCD);
    lcd.begin(ctx()->i2c()->peripherals());
    lcd.display();
    lcd.backlight();
    //lcd.backlightOff();
    lcd.clear();
    // reset lcd state
    memset(lcdState, 0, COLS * ROWS + 1);
}

void Screen::printToCanvas(int col, int row, const char *text) {
    // print text to canvs buffer without null terminator
    // if text is too long, truncate it to fit in the buffer
    int len = strlen(text);
    if (len > COLS - col) {
        len = COLS - col;
    }
    for (int i = 0; i < len; i++) {
        canvas[row * COLS + col + i] = text[i];
    }
}

void Screen::printToCanvasRpad(int col, int row, int value, int width) {
    // print value to canvas buffer with right padding
    // if value is too long, truncate it to fit in the buffer
    char text[10];
    sprintf(text, "%d", value);
    int len = strlen(text);
    if (len > width) {
        len = width;
    }
    for (int i = 0; i < len; i++) {
        canvas[row * COLS + col + i] = text[i];
    }
    for (int i = len; i < width; i++) {
        canvas[row * COLS + col + i] = ' ';
    }
}

void Screen::printToCanvasRpad(int col, int row, float value, int width) {
    // print value to canvas buffer with right padding
    // if value is too long, truncate it to fit in the buffer
    char text[10];
    sprintf(text, "%.4f", value);
    int len = strlen(text);
    if (len > width) {
        len = width;
    }
    for (int i = 0; i < len; i++) {
        canvas[row * COLS + col + i] = text[i];
    }
    for (int i = len; i < width; i++) {
        canvas[row * COLS + col + i] = ' ';
    }
}

void ScreenController::render() {
    if (screensCount == 0) {
        return;
    }
    screenStack[screensCount - 1]->render();
    ctx()->i2c()->channel(I2C_CHANNEL_LCD);
    printCanvasToLcd();
}   

void ScreenController::pushScreen(Screen *screen) {
    if (screensCount < 10) {
        screen->init(this, lcdCanvas);
        screenStack[screensCount++] = screen;
        screen_meta meta = screen->getMeta();
        if (meta.autoUpdateInterval > 0) {
            lcdRefreshTask.setInterval(meta.autoUpdateInterval);
        } else {
            lcdRefreshTask.setInterval(TASK_ONCE);
        }
        lcdRefreshTask.enable(); // render
    }
}

void ScreenController::popScreen() {
    if (screensCount > 0) {
        // free memory for the screen
        delete screenStack[screensCount - 1];
        screenStack[screensCount - 1] = nullptr;
        screensCount--;
        if (screensCount == 0) {
            lcdRefreshTask.setInterval(TASK_ONCE);
            lcdRefreshTask.disable(); // render
        } else {
            screen_meta meta = screenStack[screensCount - 1]->getMeta();
            if (meta.autoUpdateInterval > 0) {
                lcdRefreshTask.setInterval(meta.autoUpdateInterval);
            } else {
                lcdRefreshTask.setInterval(TASK_ONCE);
            }
        }
        lcdRefreshTask.restart(); // render
    }
}

Task* ScreenController::getLcdRefreshTask() {
    return &lcdRefreshTask;
}