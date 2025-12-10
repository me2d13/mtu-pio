#include "screen/MenuScreen.h"
#include "screen/InfoScreen.h"
#include "screen/AxisScreen.h"
#include "screen/ButtonsScreen.h"
#include "screen/XplScreen.h"
#include "screen/SimCtrlScreen.h"
#include "screen/SettingsScreen.h"
#include "screen/MotorIOScreen.h"
#include "context.h"
#include "Logger.h"
#include "config.h"
#include "Screen.h"

#define MENU_ITEM_COUNT 7

screen_meta MenuScreen::getMeta()
{
    screen_meta meta;
    meta.autoUpdateInterval = 1000;
    return meta;
}
void MenuScreen::render()
{
    if (ctx()->state.transient.getRotaryButtonPressedTime() > 0) {
        // rotary button pressed, do something
        ctx()->state.transient.setRotaryButtonPressedTime(0);
        if (selectedItem == 0) {
            controller->pushScreen(new InfoScreen());
        } else if (selectedItem == 1) {
            controller->pushScreen(new AxisScreen());
        } else if (selectedItem == 2) {
            controller->pushScreen(new ButtonsScreen());
        } else if (selectedItem == 3) {
            controller->pushScreen(new SettingsScreen());
        } else if (selectedItem == 4) {
            controller->pushScreen(new XplScreen());
        } else if (selectedItem == 5) {
            controller->pushScreen(new SimCtrlScreen());
        } else if (selectedItem == 6) {
            controller->pushScreen(new MotorIOScreen());
        }
        return;
    }
    int currentRotaryPos = ctx()->state.transient.getRotaryEncoderValue();
    if (currentRotaryPos != lastRotaryPos) {
        int delta = currentRotaryPos - lastRotaryPos;
        if (delta > 0) {
            selectedItem++;
            if (selectedItem >= MENU_ITEM_COUNT) {
                selectedItem = MENU_ITEM_COUNT - 1; // Hard stop at last item
            }
        } else if (delta < 0) {
            selectedItem--;
            if (selectedItem < 0) {
                selectedItem = 0; // Hard stop at first item
            }
        }
        lastRotaryPos = currentRotaryPos;
    }
    memccpy(canvas, 
        " Info      Sim data "
        " Axis      Sim ctrl "
        " Buttons   Motor IO "
        " Settings           "
        , 0, COLS * ROWS);
    // printToCanvasRpad(10, 3, currentRotaryPos, 4);
    // printToCanvasRpad(10, 3, ctx()->state.transient.getRotaryButtonPressedCount(), 4);
    int selectedIndicatorPos = (selectedItem % ROWS) * COLS;
    if (selectedItem >= ROWS) {
        selectedIndicatorPos += (COLS / 2); // second column
    }
    canvas[selectedIndicatorPos] = '>';
    canvas[selectedIndicatorPos + COLS / 2 - 1] = '<';
}