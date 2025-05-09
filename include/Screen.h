#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"

#define COLS 20
#define ROWS 4

struct screen_meta
{
    int autoUpdateInterval;
};

class ScreenController;

class Screen
{
protected:
    ScreenController *controller;
    char *canvas;
    void printToCanvas(int col, int row, const char *text);
    void printToCanvasRpad(int col, int row, int value, int width);
    void printToCanvasRpad(int col, int row, float value, int width);
public:
    void init(ScreenController *controller, char *canvas) {
        this->controller = controller;
        this->canvas = canvas;
    }
    virtual void render() = 0;
    virtual screen_meta getMeta() = 0;
};

class ScreenController
{
private:
    Screen *screenStack[10] = {nullptr};
    int screensCount = 0;
    screen_meta currentScreenMeta;
public:
    Task *getLcdRefreshTask();
    void pushScreen(Screen *screen);
    void popScreen();
    void hwSetup(); // setup HW part only
    void menu(); // setup HW part + screens
    void render();
    void showText(const char *text);
};

