#include "axis.h"
#include "AS5600.h"
#include "joy.h"
#include "config.h"
#include "context.h"

AS5600* sensor;

int axisValues[NUMBER_OF_AXIS];

void setupAxis() {
    sensor = new AS5600(ctx()->i2c()->sensors());
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