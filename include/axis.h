#pragma once
#include <Wire.h>

void readAxisData();
void setupAxis(TwoWire& i2c);
int getAxisValue(int index);
