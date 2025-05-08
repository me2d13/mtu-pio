# Motorized throttle unit v2
This is 2nd version of custom software for motorized throttle unit (MTU) running at ESP32-S3.
The unit is built based on Karl's excellent [design](https://www.737diysim.com/product-page/737-motorised-throttle-unit-mtu-v5-cad).
I did several changes in the design, hopefully improvements. These are
- using [AS5600](https://www.google.com/search?q=as5600+module) modules instead of hall sensors. For 2 reasons
  - they are much cheaper
  - they have digital output via I2C so I believe are less noisy as analog reading stays on the chip only
- instead of Arduino mega + nanos + optional joystick chip (Bodnar) I used only 1 chip ESP32 which does all the job. I also used it's wireless capabilities so the unit can (next to standard USB cable for joystick) connect to wifi and be controlled over the network. This is mainly used for configuration, logging and motor control. Disadvantage is it requires dedicated firmware which is basically this repository
- additional LCD display + rotary encoder to show real time data in some basic menu structure
- some hw features were removed to simplify the build. These were servo for parking brake and led strips
- one more sensor was added to trim axis so it can be used as trim wheel for some GA aircraft

# Hardware
- ESP32-S3 at module [YD-ESP32-S3 N16R8](https://circuitpython.org/board/yd_esp32_s3_n16r8/)
- MCP23017 to provide enough inputs for buttons
- MCP23017 to provide enough outputs
- CD4051BE to multiplex UART bus to control steppers through TMC2208

## PCB
Link to PCB in EasyEda will be added. I did several design mistakes in first version and have workarounds in place. These errors are
- wrong size for TCA9548 module. Had to add small conversion PCB and also added pull up resistors for I2C channels to be sure
- missing common ground between ESP32 and external power!
- ESP32 has 2 hardware I2C busses. I planned to use one for sensors only and second for all the rest (LCD, pin multiplexers). By wrong wiring design I used only one bus. However ESP32 seems to be fine with my timing requirements (100 sensor readings per second)
- for STEP & DIRECTION pins for steppers I used ESP32 pins directly. Later I realized many of them are reserved for USB communication and for PSRAM. Had to do some swaps in motor config as STEP pulses are needed only for trim indicators. Luckily throttles and speed brake can be controlled via UART interface. Details in `motors.h`

## Input buttons
1. Auto-throttle disconnect #1
1. Auto-throttle disconnect #2
1. TOGA #1
1. TOGA #2
1. Stab trim main select
1. Stab trim auto pilot
1. Fuel #1
1. Fuel #2
1. Parking brake
1. Horn cutoff
1. Trim indicator stop 1
1. Trim indicator stop 2
1. 12V present
1. rotary A
1. rotary B
1. rotary button


## Outputs
1. Parking brake LED
1. Stepper Throttle 1 Enabled
1. Stepper Throttle 1 Direction
1. **Stepper Throttle 1 Step**
1. Stepper Throttle 2 Enabled
1. Stepper Throttle 2 Direction
1. **Stepper Throttle 2 Step**
1. Stepper Trim 1 Enabled
1. Stepper Trim 1 Direction
1. **Stepper Trim 1 Step**
1. Stepper Trim 2 Enabled
1. Stepper Trim 2 Direction
1. **Stepper Trim 2 Step**
1. Stepper Speed brake Enabled
1. Stepper Speed brake Direction
1. **Stepper Speed brake Step**
1. Stepper Trim Wheel Enabled
1. Stepper Trim Wheel Direction
1. **Stepper Trim Wheel Step**
1. **UART addr 0**
1. **UART addr 1**
1. **UART addr 2**

## PCB requested features
- 3.3V from ESP32 board or externally (when board stab would not be strong enough)
- 5V externally from 12V or from ESP32 board

# Software
## Used libraries
- [AS5600](https://registry.platformio.org/libraries/robtillaart/AS5600) to read axes positions
- [Joystick_ESP32S2](https://registry.platformio.org/libraries/schnoog/Joystick_ESP32S2) to be a joystick HID device
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) to schedule tasks :-)
- [LCD-I2C](https://github.com/hasenradball/LCD-I2C) to control LCD display
- [ESP32RotaryEncoder](https://github.com/MaffooClock/ESP32RotaryEncoder) to use rotary encoder for menu system at LCD
- [TMCStepper](https://github.com/teemuatlut/TMCStepper) to control steppers via TMC2208
- [TCA9548](https://github.com/RobTillaart/TCA9548) to control I2C multiplexer as TMC2208s have hardcoded address
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) to handle json in API and config
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) to provide web UI + API
- [PicoCSS](https://picocss.com/docs) - for UI styling

## Lever movements based on simulator data
There's some logic involved when controlling motors of levers (throttles, speed brake) based on simulator data.
1. sometimes the movement can be turned on/off in simulator (auto throttle on/off)
1. levers are also joystick axes so changing position by motor would send update to simulator which recognizes manual interference and sends updated value back as state (which can cancel the movement)

For now as a solution there is some logic around second point so first point can be more less ignored. As side effect (of ignoring first point) the lever (e.g. throttle) position is updated also when user changes the position in simulator, e.g. using mouse in the cockpit.

So what are the implemented rules
- when levers is touched manually and new position is sent as joystick update it won't be moved by motor for some ignore period - currently one second. So if user is moving the lever actively it won't be moved by motor based on simulator value for some time
- when new value is received from the simulator and lever needs to be moved the motor starts turning and no updates are sent on joystick axis during that time - until motor stops

## USB device
As documented in Joystick_ESP32S2 library, HID device can be even turned on in platformio.ini file by `DARDUINO_USB_CDC_ON_BOOT=1` or dynamically in code. Originally I was using first option (always on) but then moved to second.

When HID is always on it's more complicated to upload new firmware to ESP32. To do it BOOT button must be pressed during reset. It will switch ESP32 to upload mode (without HID) and after upload device needs to be rebooted. It's quite complicated when BOOT button is not easily accessible. 

Another option is to use OTA which is supported in the code, but later it appeared to be not stable. Sometimes (and I didn't find out when and how) the wifi at ESP32 becomes very slow and OTA fails in the middle.

So to have easy rollback option to upload via USB, I'm turning HID on from the code. When I need to upload new firmware I turn it on (from LCD menu or web UI), reboot the device, then I can upload firmware without holding BOOT button and then start HID again.