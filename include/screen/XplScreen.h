#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"


class XplScreen : public Screen
{
private:
public:
    XplScreen() {}
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 500;
        return result;
    };

    void render() override {
        if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
            // rotary button pressed, do something
            ctx()->state.transient.setRotaryButtonPressedTime(0);
            controller->popScreen();
            return;
        }
        if (millis() - ctx()->state.transient.getSimData()->lastUpdateTime > 10000) {
            memccpy(canvas, 
                " No sim data in last"
                " 10 seconds         "
                "                    "
                "                    "
                , 0, COLS * ROWS);
            return;
        }
        memccpy(canvas, 
            "T1:        T2:      "
            "SB:        Tr:      "
            "PB:        AT:      "
            "                    "
            , 0, COLS * ROWS);
        printToCanvasRpad(3, 0, ctx()->state.transient.getSimData()->throttle1, 6);
        printToCanvasRpad(14, 0, ctx()->state.transient.getSimData()->throttle2, 6);
        printToCanvasRpad(3, 1, ctx()->state.transient.getSimData()->speedBrake, 6);
        printToCanvasRpad(14, 1, ctx()->state.transient.getSimData()->trim, 6);
        printToCanvas(3, 2, ctx()->state.transient.getSimData()->parkingBrake ? "ON" : "OFF");
        printToCanvas(14, 2, ctx()->state.transient.getSimData()->autoThrottle ? "ON" : "OFF");
    }
};

