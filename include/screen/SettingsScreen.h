#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"

class SettingsScreen : public ScreenWithMenu
{
public:
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 200;
        return result;
    };

    int getItemsCount() override {
        return 4;
    }

    void onSelect() override {
        if (selectedItem == 0) {
            ctx()->state.persisted.toggleHidOn();
        } else if (selectedItem == 1) {
            ctx()->state.persisted.enableTrimWheel = !ctx()->state.persisted.enableTrimWheel;
            ctx()->state.persisted.saveToFlash();
        } else if (selectedItem == 2) {
            // hardware reset ESP32
            // ESP.restart();
            ctx()->simDataDriver.calibrate();
        } else if (selectedItem == 3) {
            ctx()->screenController.popScreen();
        }
    }


    void doRender() override {
        memccpy(canvas, 
            " Joystick [ ]       "
            " Trim wheel [ ]     "
            " Calibrate trims    "
            " Back               "
            , 0, COLS * ROWS);
        canvas[COLS * selectedItem] = '>';
        canvas[COLS * selectedItem + COLS - 1] = '<';
        if (ctx()->state.persisted.isHidOn) {
            canvas[11] = 'X';
        }
        if (ctx()->state.persisted.enableTrimWheel) {
            canvas[COLS + 13] = 'X';
        }
    }
};

