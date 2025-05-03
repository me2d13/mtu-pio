#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"


class ButtonsScreen : public Screen
{
private:
public:
    ButtonsScreen() {}
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
        memccpy(canvas, 
            " Buttons raw value  "
            "                    "
            "                    "
            "                    "
            , 0, COLS * ROWS);
        int rawValues = ctx()->state.transient.getButtonsRawValue();
        // convert 16 bits to binary string
        char binaryString[17];
        for (int i = 0; i < 16; i++) {
            binaryString[15 - i] = (rawValues & (1 << i)) ? '1' : '0';
        }
        binaryString[16] = '\0'; // null-terminate the string
        printToCanvas(1, 2, binaryString);
    }
};

