#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"


class InfoScreen : public Screen
{
private:
public:
    InfoScreen() {}
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 1000; // 1 second
        return result;
    };

    void render() override {
        if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
            // rotary button pressed, do something
            ctx()->state.transient.setRotaryButtonPressedTime(0);
            controller->popScreen();
            return;
        }
        memccpy(canvas, 
            "IP :                "
            "      M  T  U       "
            "T1: ????   T2: ???? "
            "                    "
            , 0, COLS * ROWS);
        printToCanvas(4, 0, getIp().c_str());
        printToCanvasRpad(4, 2, ctx()->state.transient.getAxisValue(1), 4);
        printToCanvasRpad(15, 2, ctx()->state.transient.getAxisValue(2), 4);
        printToCanvas(0, 3, getTimeStr().c_str());
    }
};

