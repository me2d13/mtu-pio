#include <Joystick_ESP32S2.h>
#include "Logger.h"
#include "config.h"

Joystick_ joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 32, 0,
  true, true, true, true, true, true, // x, y, z, rx, ry, rz
  false, false, false, false, false); 

int lastAxis[NUMBER_OF_AXIS];
u_char lastButtons[NUMBER_OF_BUTTONS];
bool dirty = false;

void setupJoy() {
/*    USB.PID(0xDDFD);
    USB.VID(0x5053);
    USB.productName("MTU_v3");
    USB.manufacturerName("Me2d");
    USB.begin();
    */
   logger.log("Setting up joystick...");
    joystick.setXAxisRange(0, 4095);
    joystick.setYAxisRange(0, 4095);
    joystick.setZAxisRange(0, 4095);
    joystick.setRxAxisRange(0, 4095);
    joystick.setRyAxisRange(0, 4095);
    joystick.setRzAxisRange(0, 4095);
    joystick.begin(false);
   logger.log("Joystick started");
   joystick.sendState();
   for (int i = 0; i < NUMBER_OF_AXIS; i++) lastAxis[i] = 0;
   for (int i = 0; i < NUMBER_OF_BUTTONS; i++) lastButtons[i] = 0;
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