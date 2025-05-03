#pragma once
#include "Screen.h"

class MenuScreen : public Screen
{
private:
    int selectedItem = 0;
    int lastRotaryPos = 0;
public:
    MenuScreen(int rotaryPosition) : lastRotaryPos(rotaryPosition) {}
    screen_meta getMeta() override;
    void render() override;
};

