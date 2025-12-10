# AI Agent Context for MTU v2 Project

This document provides context for AI coding assistants working on this project.

## Project Overview

**Motorized Throttle Unit (MTU) v2** - Custom firmware for ESP32-S3 based motorized throttle quadrant for Boeing 737 flight simulator.

### Hardware Platform
- **MCU**: ESP32-S3 (YD-ESP32-S3 N16R8 module)
- **Display**: 20x4 LCD (I2C)
- **Input**: Rotary encoder with push button
- **Sensors**: 5x AS5600 magnetic encoders (I2C) for axis position
- **Motors**: 6x stepper motors controlled via TMC2208 drivers (UART)
- **I/O Expansion**: 2x MCP23017 (I2C) for buttons and outputs
- **I2C Multiplexing**: TCA9548 for multiple I2C devices with same address

### Key Features
1. **USB Joystick (HID)** - Acts as game controller, sends axis/button data to PC
2. **WiFi Connectivity** - Connects to LAN for configuration and control
3. **UDP Listener** - Receives simulator data (X-Plane/MSFS) to control motors
4. **Web Interface** - HTTP server for monitoring, configuration, and control
5. **LCD Menu System** - Local interface using 20x4 display + rotary encoder
6. **Persistent Configuration** - JSON config stored in SPIFFS (`/config.json`)

### Physical Controls
**Axes (7 total):**
- Speed brake lever
- Throttle 1 & 2 levers
- Flaps lever
- Trim wheel
- Reverse thrust 1 & 2 (mapped to joystick buttons when >50%)

**Buttons (16 total):**
- Auto-throttle disconnect #1 & #2
- TOGA #1 & #2
- Stabilizer trim switches (main, auto-pilot)
- Fuel cutoff #1 & #2
- Parking brake
- Horn cutoff
- Trim indicator end stops #1 & #2
- 12V power present detection
- Rotary encoder A, B, button

**Motors (6 total):**
- Throttle 1 & 2 (motorized, follow sim data)
- Speed brake (motorized, follow sim data)
- Trim wheel (optional motor, spins when trim changes)
- Trim indicators 1 & 2 (needles showing trim position, always active)

## Project Structure

```
mtu-pio/
├── include/
│   ├── config.h              # Pin definitions, hardware config
│   ├── state.h               # State management (persisted & transient)
│   ├── Screen.h              # LCD screen base classes
│   └── screen/               # Individual LCD screen headers
│       ├── MenuScreen.h      # Main menu (7 items, 2 columns)
│       ├── InfoScreen.h      # System info display
│       ├── AxisScreen.h      # Axis values display
│       ├── ButtonsScreen.h   # Button states display
│       ├── SettingsScreen.h  # Settings (Joystick, Calibrate, Back)
│       ├── MotorIOScreen.h   # Motor enable/disable (Trim wheel, Speed brake)
│       ├── XplScreen.h       # X-Plane specific data
│       └── SimCtrlScreen.h   # Simulator control data
├── src/
│   ├── main.cpp              # Main entry point
│   ├── state.cpp             # State serialization/persistence
│   ├── lcd.cpp               # LCD rendering engine
│   ├── SimDataDriver.cpp     # Motor control logic based on sim data
│   ├── motors.cpp            # Motor controller
│   ├── axis.cpp              # Axis reading and calibration
│   ├── joy.cpp               # USB HID joystick
│   ├── udp.cpp               # UDP listener for sim data
│   ├── web.cpp               # Web server
│   ├── api.cpp               # REST API endpoints
│   ├── net.cpp               # WiFi management
│   └── screen/
│       └── MenuScreen.cpp    # Main menu implementation
├── data/www/                 # Web interface files (served from SPIFFS)
│   ├── config.html           # Configuration page
│   └── config.js             # Configuration page logic
└── platformio.ini            # PlatformIO build configuration
```

## Configuration System

### PersistedState (`include/state.h`, `src/state.cpp`)

Configuration that survives reboots, stored in `/config.json` on SPIFFS:

```cpp
class PersistedState {
    axis_settings axisSettings[7];      // Min/max calibration for each axis
    motor_settings motorSettings[6];    // Current, microsteps, speed multiplier
    bool isHidOn;                       // USB HID joystick enabled
    int trimWheelVelocity;              // Trim wheel motor speed
    bool enableTrimWheel;               // Enable trim wheel motor
    bool enableSpeedBrake;              // Enable speed brake motor
};
```

**JSON Serialization:**
- `fillJsonDocument()` - Export to JSON
- `loadFromJsonObject()` - Import from JSON
- `saveToFlash()` / `loadFromFlash()` - Persist to SPIFFS
- Accessible via web API at `/api/state`

### TransientState

Runtime state (not persisted):
- Current axis values (raw & calibrated)
- Button states
- Simulator data (throttle, trim, speed brake, parking brake, etc.)
- Rotary encoder position
- Error counters

## Motor Control Logic

Located in `src/SimDataDriver.cpp`:

### Control Modes
- **FREE** - Motor idle, joystick updates sent normally
- **CHASE** - Motor moving to match sim data, joystick updates suppressed

### Key Drivers

**ThrottleDriver** (2 instances):
- Only moves when auto-throttle is ON
- Respects `MOTORIZED_UPDATE_IGNORE_INTERVAL` (1 second) after manual movement

**SpeedBrakeDriver**:
- Moves when `enableSpeedBrake` is true
- Stops motor and sets FREE mode when disabled

**TrimDriver**:
- Controls 2 trim indicator needles (always active)
- Optionally spins trim wheel when `enableTrimWheel` is true
- Has calibration routine using end-stop switches

## LCD Menu System

**Display**: 20 columns × 4 rows

**Screen Architecture:**
- `Screen` - Base class with canvas rendering
- `ScreenWithMenu` - Adds rotary encoder navigation
- `ScreenController` - Manages screen stack, auto-refresh

**Main Menu Layout:**
```
 Info      Sim data 
 Axis      Sim ctrl 
 Buttons   Motor IO 
 Settings           
```

**Navigation:**
- Rotate encoder to move selection
- Press encoder button to select
- Screens can push/pop from stack

## Build & Upload

### Build System
- **PlatformIO** (installed in `%USERPROFILE%\.platformio\penv\Scripts\`)
- **Platform**: espressif32
- **Board**: esp32-s3-devkitc-1
- **Framework**: Arduino

### Commands

**Build:**
```bash
platformio run
```

**Upload (PREFERRED - OTA):**
```bash
platformio run -e esp32-s3-ota -t upload
```
Device hostname: `mtu.local`

**Upload (USB - fallback):**
```bash
platformio run -e esp32-s3-usb -t upload
```
USB port: COM11

**Note:** When asked to "upload" without specifying method, use OTA.

### OTA Upload Requirements
- Device must be powered and connected to WiFi
- Device must be reachable at `mtu.local` on the network
- HID can be enabled during OTA (doesn't interfere)

## Common Development Tasks

### Adding a New Configuration Option

1. **Add field to `PersistedState`** in `include/state.h`
2. **Add to JSON serialization** in `src/state.cpp`:
   - `fillJsonDocument()` - export
   - `loadFromJsonObject()` - import
   - `resetToDefaultValues()` - set default
3. **Add to web interface** (optional) in `data/www/config.js`
4. **Add to LCD menu** (optional) - create/modify screen in `include/screen/`

### Adding a New LCD Screen

1. **Create header** in `include/screen/YourScreen.h`
2. **Inherit from** `Screen` or `ScreenWithMenu`
3. **Implement required methods:**
   - `getMeta()` - set auto-refresh interval
   - `render()` or `doRender()` - draw to canvas
   - `getItemsCount()` / `onSelect()` - if using menu
4. **Add to menu** in `src/screen/MenuScreen.cpp`

### Motor Control Pattern

When adding motor enable/disable logic:
```cpp
void MotorDriver::dataChanged(float oldValue, float newValue) {
    if (!ctx()->state.persisted.enableMotor) {
        // Motor disabled - stop if running
        if (state.controlMode == CHASE) {
            ctx()->motorsController.getMotor(motorIndex)->stopMotor();
            state.controlMode = FREE;
        }
        return;
    }
    // Normal motor control logic...
}
```

## Important Notes

### HID Device Management
- HID can be enabled/disabled from code (not compile-time)
- When HID is ON, uploading via USB requires holding BOOT button during reset
- OTA upload works regardless of HID state
- Toggle HID from Settings menu or web interface

### Motor Movement Logic
- Motors ignore sim updates for 1 second after manual lever movement
- Joystick updates suppressed while motor is moving (CHASE mode)
- This prevents feedback loops between manual input and sim response

### I2C Bus Architecture
- Originally planned for 2 I2C buses, but design error uses only one
- All I2C devices share single bus (works fine with current timing)
- TCA9548 multiplexer used for TMC2208 drivers (same I2C address)

### Calibration
- Axis calibration: min/max values stored per axis
- Trim indicators: auto-calibration using end-stop switches
- Calibration accessible from Settings → Calibrate trims

## Libraries Used

- **AS5600** - Magnetic encoder reading
- **Joystick_ESP32S2** - USB HID joystick
- **TaskScheduler** - Cooperative multitasking
- **LCD-I2C** - LCD display control
- **ESP32RotaryEncoder** - Rotary encoder input
- **TMCStepper** - Stepper motor driver control
- **TCA9548** - I2C multiplexer
- **ArduinoJson** - JSON parsing/serialization
- **ESPAsyncWebServer** - Web server
- **PicoCSS** - Web UI styling

## Troubleshooting

### Build Issues
- Ensure PlatformIO is in PATH or use full path
- Check `platformio.ini` for correct board/platform versions

### Upload Issues
- **OTA fails**: Check WiFi connection, try `ping mtu.local`
- **USB fails**: Hold BOOT button during reset if HID is enabled
- **Slow OTA**: Known issue, fallback to USB if needed

### Runtime Issues
- Check serial monitor for logs (115200 baud)
- Web interface shows current state at `/api/state`
- LCD Info screen shows system status

## Web API Endpoints

- `GET /api/state` - Get current state (persisted + transient)
- `POST /api/state` - Update persisted state (JSON body)
- `POST /api/state` with `{"factoryReset": true}` - Reset to defaults

## Design Philosophy

- **Modularity**: Screens, drivers, and controllers are separate
- **State Management**: Clear separation of persisted vs transient state
- **Fail-Safe**: Motors can be disabled, HID can be toggled
- **User Control**: Both web and LCD interfaces for all settings
- **Persistence**: All configuration survives reboots

---

**Last Updated**: 2025-12-10  
**Firmware Version**: v2 (ESP32-S3 based)
