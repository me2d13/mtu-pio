#include <Joystick_ESP32S2.h>
#include "Logger.h"
#include "config.h"
#include "context.h"
#include "axis.h"

/* 

When joystick is enabled in platformio.ini, I have new COM8 Serial + HID device. Upload via OTA or boot button
Without joystick, I have COM11 Serial as upload port. 
COM9 is present in both scenarios
*/

Joystick_ joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, NUMBER_OF_BUTTONS, 0,
  true, true, true, true, true, true, // x, y, z, rx, ry, rz
  true, false, false, false, false); // rudder, throttle, accelerator, brake, steering

int lastAxis[NUMBER_OF_AXIS];
int lastRawButtons = 0;
bool dirty = false;

void setJoyAxis(int index, int value);
void sendJoy();

void setupJoy() {
/*    USB.PID(0xDDFD);
    USB.VID(0x5053);
    USB.productName("MTU_v3");
    USB.manufacturerName("Me2d");
    USB.begin();
    */
   logger.log("Setting up joystick...");
    joystick.setXAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setYAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setZAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setRxAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setRyAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setRzAxisRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.setRudderRange(0, AXIS_MAX_CALIBRATED_VALUE);
    joystick.begin(false);
   logger.log("Joystick started");
   joystick.sendState();
   for (int i = 0; i < NUMBER_OF_AXIS; i++) lastAxis[i] = 0;
}

void readStateDataAndSendJoy() {
    // read axis data from the sensors
    for (int i = 0; i < NUMBER_OF_AXIS; i++) {
        int value = ctx()->state.transient.getAxisValue(i);
        if (value != -1) {
            setJoyAxis(i, ctx()->state.transient.getCalibratedAxisValue(i, &ctx()->state.persisted.axisSettings[i]));
        }
    }
    if (lastRawButtons != ctx()->state.transient.getButtonsRawValue()) {
        dirty = true;
        lastRawButtons = ctx()->state.transient.getButtonsRawValue();
        for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
            int value = (lastRawButtons >> i) & 0x01;
            joystick.setButton(i, value);
        }
    }
    sendJoy();
}

void setJoyAxis(int index, int value) {
  bool changed = (value != lastAxis[index]);
  if (changed) {
    dirty = true;
    lastAxis[index] = value;
    switch (index)
    {
    case X_AXIS:
      joystick.setXAxis(value);
      break;
    
    case Y_AXIS:
      joystick.setYAxis(value);
      break;
    
    case Z_AXIS:
      joystick.setZAxis(value);
      break;
    
    case RX_AXIS:
      joystick.setRxAxis(value);
      break;
    
    case RY_AXIS:
      joystick.setRyAxis(value);
      break;

    case RZ_AXIS:
      joystick.setRzAxis(value);
      break;

    case BRAKE_AXIS:
      joystick.setRudder(value);
      break;
    
    default:
      break;
    }
  }
}

void sendJoy() {
  if (dirty) {
    joystick.sendState();
    dirty = false;
  }
}