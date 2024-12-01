#include "axis.h"
#include "AS5600.h"
#include "joy.h"
#include "config.h"

AS5600* sensor;

int axisValues[NUMBER_OF_AXIS];

void setupAxis(TwoWire& i2c) {
    sensor = new AS5600(&i2c);
    sensor->begin();
}

void readAxisData() {
    int value = sensor->rawAngle();
    if (sensor->lastError() == AS5600_OK) {
        axisValues[0] = value;
    }
}

int getAxisValue(int index) {
    return axisValues[index];
}