#pragma once
#include "Screen.h"
#include "context.h"

class MotorIOScreen : public ScreenWithMenu
{
public:
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 200;
        return result;
    };

    int getItemsCount() override {
        return 3;
    }

    void onSelect() override {
        if (selectedItem == 0) {
            ctx()->state.persisted.enableTrimWheel = !ctx()->state.persisted.enableTrimWheel;
            ctx()->state.persisted.saveToFlash();
        } else if (selectedItem == 1) {
            ctx()->state.persisted.enableSpeedBrake = !ctx()->state.persisted.enableSpeedBrake;
            ctx()->state.persisted.saveToFlash();
        } else if (selectedItem == 2) {
            ctx()->screenController.popScreen();
        }
    }


    void doRender() override {
        memccpy(canvas, 
            " Trim wheel [ ]     "
            " Speed brake [ ]    "
            " Back               "
            "                    "
            , 0, COLS * ROWS);
        canvas[COLS * selectedItem] = '>';
        canvas[COLS * selectedItem + COLS - 1] = '<';
        if (ctx()->state.persisted.enableTrimWheel) {
            canvas[13] = 'X';
        }
        if (ctx()->state.persisted.enableSpeedBrake) {
            canvas[COLS + 14] = 'X';
        }
    }
};

