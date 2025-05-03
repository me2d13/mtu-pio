#include <Arduino.h>
#include "rotary.h"
#include "Logger.h"
#include "context.h"
#include "state.h"

/* Code in encoder library was changed slightly as reported in issue: https://github.com/MaffooClock/ESP32RotaryEncoder/issues/11

This is changed code:

void ARDUINO_ISR_ATTR RotaryEncoder::_button_ISR()
{
  portENTER_CRITICAL_ISR( &mux );
  int pinValue = digitalRead( encoderPinButton );

  static unsigned long _lastInterruptTime = 0;
  static unsigned int _lastPinValue = -1;

  // Simple software de-bounce
  bool isBouncing = (( millis() - _lastInterruptTime ) < 30);
  bool isSameValue = ( pinValue == _lastPinValue );
  if( isBouncing || isSameValue ) {
    portEXIT_CRITICAL_ISR( &mux );
    return;
  }
  _lastPinValue = pinValue;

  // HIGH = idle, LOW = active
  bool isPressed = !pinValue;

*/

void EncoderInput::setup()
{
    // set up rotary encoder
    rotaryEncoder.setBoundaries(-10000, 10000); // set boundaries for the encoder
    rotaryEncoder.onTurned([&](long value) {
        knobCallback(value);
    });
    rotaryEncoder.onPressed([&](unsigned long duration) {
        buttonCallback(duration);
    });
    rotaryEncoder.setEncoderType(FLOATING); // set encoder type to floating
    rotaryEncoder.begin();
    rotaryChangeTask = ctx()->screenController.getLcdRefreshTask();

    logger.log("Rotary encoder setup done");
}

void EncoderInput::knobCallback(long value)
{
    ctx()->state.transient.setRotaryEncoderValue(value);
    if (rotaryChangeTask != nullptr) {
        rotaryChangeTask->restart();
    }
    //String message = "Rotary encoder knob value: " + String(value);
    //logger.log(message.c_str());
}

void EncoderInput::buttonCallback(unsigned long duration)
{
    ctx()->state.transient.setRotaryButtonPressedTime(duration);
    ctx()->state.transient.setRotaryButtonPressedCount(ctx()->state.transient.getRotaryButtonPressedCount() + 1);
    if (rotaryChangeTask != nullptr) {
        rotaryChangeTask->restart();
    }
    //String message = "Rotary encoder button pressed for: " + String(duration) + " ms";
    //logger.log(message.c_str());
}