#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"

#define ITEMS_COUNT 3

class SettingsScreen : public Screen
{
private:
    int lastRotaryPos;
    int selectedItem = 0;
public:
    SettingsScreen() {
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
                selectedItem++;
                if (selectedItem >= ITEMS_COUNT) {
                    selectedItem = ITEMS_COUNT - 1;
                }
            } else {
                selectedItem--;
                if (selectedItem < 0) {
                    selectedItem = 0;
                }
            }
            lastRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
        }
        if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
            // rotary button pressed, do something
            ctx()->state.transient.setRotaryButtonPressedTime(0);
            if (selectedItem == 0) {
                ctx()->state.persisted.toggleHidOn();
            } else if (selectedItem == 1) {
                // hardware reset ESP32
                ESP.restart();
            } else if (selectedItem == 2) {
                ctx()->screenController.popScreen();
            }
            return;
        }
        memccpy(canvas, 
            " Joystick [ ]       "
            " Reset              "
            " Back               "
            "                    "
            , 0, COLS * ROWS);
        canvas[COLS * selectedItem] = '>';
        canvas[COLS * selectedItem + COLS - 1] = '<';
        if (ctx()->state.persisted.isHidOn) {
            canvas[11] = 'X';
        }
    }
};

