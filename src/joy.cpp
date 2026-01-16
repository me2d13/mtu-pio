#include <Joystick_ESP32S2.h>
#include "Logger.h"
#include "config.h"
#include "context.h"
#include "axis.h"

#define ALLOW_REVERSE_BLOCKING

#define xxLOG_TRIM

/* 

When joystick is enabled in platformio.ini, I have new COM8 Serial + HID device. Upload via OTA or boot button
Without joystick, I have COM11 Serial as upload port. 
COM9 is present in both scenarios
*/

Joystick_ joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, NUMBER_OF_BUTTONS + NUMBER_OF_VIRTUAL_BUTTONS, 0, // +2 for reverse buttons (derived from axis) + 2 for trim buttons
  true, true, true, true, true, true, // x, y, z, rx, ry, rz
  true, false, false, false, false); // rudder, throttle, accelerator, brake, steering

int lastAxis[NUMBER_OF_AXIS];
int lastRawButtons = 0;
bool dirty = false;
int lastTrimValue = -1;
int currentTrimValue = -1;
int lastTrimPositionInSim = 0;
unsigned long lastTrimButtonPressTime = 0;
unsigned long lastTrimButtonReleaseTime = 0;


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

void changeTrimAxisToButtons(int value) {
  // handle init
  if (lastTrimValue < 0) {
    lastTrimValue = value;
    return;
  }

  // calculate diff from last reported value taking override into account
  // suppose increase +10, works till new is 9999 and old 9989. Then new gets 9 and old 9999 so we would get -9990
  // suppose decrease -10, works till new is 1 and old 11. Then new gets 9991 and old 1 so we would get 9990
  int diff = value - lastTrimValue;
  if (diff < -(AXIS_MAX_CALIBRATED_VALUE/2)) {
    diff += AXIS_MAX_CALIBRATED_VALUE;
  } else if (diff > (AXIS_MAX_CALIBRATED_VALUE/2)) {
    diff -= AXIS_MAX_CALIBRATED_VALUE;
  }
  int absDiff = (diff > 0) ? diff : -diff;
  bool haveBigDiff = (absDiff > TRIM_SENSOR_THRESHOLD);
  if (haveBigDiff) {
    int newTrimValue = ctx()->state.transient.getTrimWheelPosition() + (diff / 10);
    ctx()->state.transient.setTrimWheelPosition(newTrimValue);
    lastTrimValue = value;
  }
  if (haveBigDiff) {
    #ifdef LOG_TRIM
        logger.print("Have trim change from ");
        logger.print(lastTrimValue);
        logger.print(" to ");
        logger.print(value);
        logger.print(" diff: ");
        logger.print(diff);
        logger.print(", abs trim position: ");
        logger.println(ctx()->state.transient.getTrimWheelPosition());
    #endif
  }
}

void handleTrimButtons() {
  unsigned long now = millis();
  // if trim button is pressed for more than TRIM_BUTTON_PRESS_MS, release it
  if (lastTrimButtonPressTime > 0 && now - lastTrimButtonPressTime > TRIM_BUTTON_PRESS_MS) {
    joystick.setButton(TRIM_UP_BUTTON_INDEX, 0);
    joystick.setButton(TRIM_DOWN_BUTTON_INDEX, 0);
    lastTrimButtonReleaseTime = now;
    lastTrimButtonPressTime = 0;
    return;
  }
  // if we are shortly after release, do nothing
  if (lastTrimButtonReleaseTime > 0) {
    if (now - lastTrimButtonReleaseTime < TRIM_BUTTON_PRESS_MS) {
      return;
    }
    lastTrimButtonReleaseTime = 0;
  }

  int trimWheelPosition = ctx()->state.transient.getTrimWheelPosition();
  // if there is a difference between my trim wheel position and the one in sim, send the buttons
  // only if the difference is big enough
  int absDiff = abs(trimWheelPosition - lastTrimPositionInSim);
  if (absDiff > TRIM_POSITION_THRESHOLD) {
    bool upDirection = trimWheelPosition > lastTrimPositionInSim;
    joystick.setButton(upDirection ? TRIM_UP_BUTTON_INDEX : TRIM_DOWN_BUTTON_INDEX, 1);
    // if we get to the tolerance with the press, mark sim position as current
    if (absDiff < 2*TRIM_POSITION_THRESHOLD) {
      lastTrimPositionInSim = trimWheelPosition;
    } else {
      // if we are far from the tolerance, just update sim handled value with direction but only about threshold
      // so we produce enough button presses to get to the tolerance
      lastTrimPositionInSim += upDirection ? TRIM_POSITION_THRESHOLD : -TRIM_POSITION_THRESHOLD;
    }
    lastTrimButtonPressTime = now;
    return;
  }
  
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
    changeTrimAxisToButtons(currentTrimValue);
    handleTrimButtons();
    sendJoy();
}

uint8_t getReverseButtonValueByCalibratedAxisValue(int axisValue) {
  return axisValue > AXIS_MAX_CALIBRATED_VALUE / 2;
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
      joystick.setButton(REV_1_BUTTON_INDEX, getReverseButtonValueByCalibratedAxisValue(value));
      break;
    
    case RY_AXIS:
      if (canSendReverse) {
        joystick.setRyAxis(value); // rev 2
      } else {
        joystick.setRyAxis(0); // rev 2
      }
      joystick.setButton(REV_2_BUTTON_INDEX, getReverseButtonValueByCalibratedAxisValue(value));
      break;

    case RZ_AXIS:
      joystick.setRudder(value);
      break;

    case BRAKE_AXIS:
      joystick.setRzAxis(value);
      break;
    
    default:
      break;
    }
    if (index == AXIS_INDEX_TRIM) {
      currentTrimValue = value;
    }
  }
}

void sendJoy() {
  if (dirty) {
    joystick.sendState();
    dirty = false;
  }
}