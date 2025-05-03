#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"


class AxisScreen : public Screen
{
private:
    int lastRotaryPos;
    bool rawValues = true;
public:
    AxisScreen() {
        lastRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
    }
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 200;
        return result;
    };

    void render() override {
        if (lastRotaryPos != ctx()->state.transient.getRotaryEncoderValue()) {
            lastRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
            rawValues = !rawValues;
        }
        if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
            // rotary button pressed, do something
            ctx()->state.transient.setRotaryButtonPressedTime(0);
            controller->popScreen();
            return;
        }
        memccpy(canvas, 
            "T1: ----- T2: ----- "
            "SB: ----- FL: ----- "
            "TR: -----           "
            " raw     calibrated "
            , 0, COLS * ROWS);
        int t1 = rawValues ? ctx()->state.transient.getAxisValue(1) : ctx()->state.transient.getCalibratedAxisValue(1, &ctx()->state.persisted.axisSettings[1]);
        int t2 = rawValues ? ctx()->state.transient.getAxisValue(2) : ctx()->state.transient.getCalibratedAxisValue(2, &ctx()->state.persisted.axisSettings[2]);
        int sb = rawValues ? ctx()->state.transient.getAxisValue(0) : ctx()->state.transient.getCalibratedAxisValue(0, &ctx()->state.persisted.axisSettings[0]);
        int fl = rawValues ? ctx()->state.transient.getAxisValue(3) : ctx()->state.transient.getCalibratedAxisValue(3, &ctx()->state.persisted.axisSettings[3]);
        int tr = rawValues ? ctx()->state.transient.getAxisValue(4) : ctx()->state.transient.getCalibratedAxisValue(4, &ctx()->state.persisted.axisSettings[4]);
        printToCanvasRpad(4, 0, t1, 5);
        printToCanvasRpad(14, 0, t2, 5);
        printToCanvasRpad(4, 1, sb, 5);
        printToCanvasRpad(14, 1, fl, 5);
        printToCanvasRpad(4, 2, tr, 5);
        if (rawValues) {
            printToCanvas(0, 3, ">raw<");
        } else {
            printToCanvas(8, 3, ">calibrated<");
        }
    }
};

