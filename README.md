# Motorised throttle unit v2
This is 2nd version of custom software for motorised throttle unit (MTU) running at ESP32-S3.

# Used libraries
- [AS5600](https://registry.platformio.org/libraries/robtillaart/AS5600)
- [Joystick_ESP32S2](https://registry.platformio.org/libraries/schnoog/Joystick_ESP32S2)
- [ReactESP](https://registry.platformio.org/libraries/mairas/ReactESP)
- [ESP Async WebServer](https://registry.platformio.org/libraries/mathieucarbou/ESP%20Async%20WebServer) maybe will be replaced by [esp32_https_server](https://registry.platformio.org/libraries/fhessel/esp32_https_server)
- [PicoCSS](https://picocss.com/docs)


# Hardware

- ESP32-S3 at module [YD-ESP32-S3 N16R8](https://circuitpython.org/board/yd_esp32_s3_n16r8/)
- MCP23017 to provide enough inputs for buttons
- MCP23017 to provide enough outputs
- CD4051BE to multiplex UART bus to control steppers through TMC2208

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
1. 5V present
1. rotary A
1. rotary B
1. roatry button


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