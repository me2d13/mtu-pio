#pragma once
#include "Screen.h"
#include "context.h"
#include "net.h"

class InfoScreen : public ScreenWithMenu
{
public:
    screen_meta getMeta() override { 
        screen_meta result; 
        result.autoUpdateInterval = 1000; // 1 second
        return result;
    };

    int getItemsCount() override {
        return 2;
    }

    void onSelect() override {
        if (selectedItem == 1) {
            setupWifi();
        } else if (selectedItem == 0) {
            controller->popScreen();
        }
    }

    void doRender() override {
        memccpy(canvas, 
            "IP :                "
            "      M  T  U       "
            "                    "
            " Back  Restart WiFi "
            , 0, COLS * ROWS);
        printToCanvas(4, 0, getIp().c_str());
        printToCanvas(0, 2, getTimeStr().c_str());
        if (selectedItem == 0) {
            canvas[3*COLS] = '>';
            canvas[3*COLS + 5] = '<';
        } else {
            canvas[3*COLS + 6] = '>';
            canvas[3*COLS + 19] = '<';
        }
    }
};

