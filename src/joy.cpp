#include <Joystick_ESP32S2.h>
#include "Logger.h"
#include "config.h"
#include "context.h"
#include "axis.h"

#define ALLOW_REVERSE_BLOCKING

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

// reverse buttons 4 and 12 as joystick values
//TODO: maybe put this to persisted settings
int reversedButtonsMask = 0b0000100000001000; 

void setJoyAxis(int index, int value);
void sendJoy();

void setupJoy() {
  if (ctx()->state.persisted.isHidOn) {
    logger.log("Joystick is enabled in settings, enabling hid.");
    USB.PID(0xDDFD);
    USB.VID(0x5056);
    USB.productName("MTU_v6");
    USB.manufacturerName("Me2d");
    USB.begin();
  } else {
    logger.log("Joystick is disabled in settings, skipping setup.");
    return;
  }
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
    if (!ctx()->state.persisted.isHidOn) {
        return; // joystick is disabled in settings
    }
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
            if (reversedButtonsMask & (1 << i)) {
                value = !value;
            }
            joystick.setButton(i, value);
        }
    }
    sendJoy();
}

void setJoyAxis(int index, int value) {
  bool changed = (value != lastAxis[index]);
  if (changed) {
    #ifdef ALLOW_REVERSE_BLOCKING
    // this is ugly hw hack. Reverses are noisy and they are disconnecting A/T in xplane Zibo
    // let's try to send them only if stab trim AP is on (which I don't have mapped in XPL)
    // for flight I'll switch it off to make sure there's no reverse value sent to XPL
    bool canSendReverse = ctx()->state.transient.getButtonsRawValue()  & (1 << 8);
    #else
    bool canSendReverse = true
    #endif
    if (!ctx()->simDataDriver.canSendJoyValue(index)) {
      // we don't send update when axis is being moved by motor
      // in that case we also don't want to set joyAxisUpdated
      return;
    }
    // set joyAxisUpdated but use 0.1% tolerance for axis value change
    float delta = abs(value - lastAxis[index]);
    if (delta > 0.001 * (float) AXIS_MAX_CALIBRATED_VALUE) {
      ctx()->state.transient.joyAxisUpdated(index);
    }
    dirty = true;
    lastAxis[index] = value;
    switch (index)
    {
    case X_AXIS:
      joystick.setXAxis(value); // throttle 1
      break;
    
    case Y_AXIS:
      joystick.setYAxis(value); // throttle 2
      break;
    
    case Z_AXIS:
      joystick.setZAxis(value); // flaps
      break;
    
    case RX_AXIS:
      if (canSendReverse) {
        joystick.setRxAxis(value); // rev 1
      } else {
        joystick.setRxAxis(0); // rev 1
      }
      break;
    
    case RY_AXIS:
      if (canSendReverse) {
        joystick.setRyAxis(value); // rev 2
      } else {
        joystick.setRyAxis(0); // rev 2
      }
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