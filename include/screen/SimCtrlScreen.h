#pragma once
#include "Screen.h"
#include "context.h"
#include "SimDataDriver.h"

#define SUB_PAGES_COUNT 4

class SimCtrlScreen : public Screen
{
private:
    int lastRotaryPos;
    int subPage = 0;
public:
    SimCtrlScreen() {
        lastRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
    }
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 200;
        return result;
    };

    void render() override {
        if (lastRotaryPos != ctx()->state.transient.getRotaryEncoderValue()) {
            int delta = ctx()->state.transient.getRotaryEncoderValue() - lastRotaryPos;
            if (delta > 0) {
                subPage++;
                if (subPage >= SUB_PAGES_COUNT) {
                    subPage = 0;
                }
            } else {
                subPage--;
                if (subPage < 0) {
                    subPage = SUB_PAGES_COUNT - 1;
                }
            }
            lastRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
        }
        if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
            // rotary button pressed, do something
            ctx()->state.transient.setRotaryButtonPressedTime(0);
            controller->popScreen();
            return;
        }
        memccpy(canvas, 
            "==                =="
            "Req:----- cal ----- "
            "Cur:----- cal ----- "
            "St: -----           "
            , 0, COLS * ROWS);
        driver_state *state = ctx()->simDataDriver.getState(subPage);
        printToCanvas(3, 0, state->name);
        printToCanvasRpad(4, 1, state->requestedValue, 5);
        printToCanvasRpad(14, 1, state->requestedPosition, 5);
        printToCanvasRpad(4, 2, state->currentValue, 5);
        printToCanvasRpad(14, 2, state->currentPosition, 5);
        if (state->controlMode == FREE) {
            printToCanvas(4, 3, "FREE ");
        } else if (state->controlMode == CHASE) {
            printToCanvas(4, 3, "CHASE ");
        } else {
            printToCanvas(4, 3, "???? ");
        }
        if (subPage == 2) {
            printToCanvas(10, 3, "CaliPh:");
            printToCanvasRpad(18, 3, ctx()->simDataDriver.trim->getCalibrationPhase(), 1);
        }
    }
};

