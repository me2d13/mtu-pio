#include <Arduino.h>
#include "rotary.h"
#include "Logger.h"
#include "context.h"
#include "state.h"

void EncoderInput::setup()
{
    // set up rotary encoder
    rotaryEncoder.setBoundaries(-100, 100); // set boundaries for the encoder
    rotaryEncoder.onTurned([&](long value) {
        knobCallback(value);
    });
    rotaryEncoder.onPressed([&](unsigned long duration) {
        buttonCallback(duration);
    });
    rotaryEncoder.setEncoderType(FLOATING); // set encoder type to floating
    rotaryEncoder.begin();

    logger.log("Rotary encoder setup done");
}

void EncoderInput::knobCallback(long value)
{
    ctx()->state.transient.setRotaryEncoderValue(value);
    if (rotaryChangeTask != nullptr) {
        rotaryChangeTask->forceNextIteration();
    }
    //String message = "Rotary encoder knob value: " + String(value);
    //logger.log(message.c_str());
}

void EncoderInput::buttonCallback(unsigned long duration)
{
    ctx()->state.transient.setRotaryButtonPressedTime(duration);
    if (rotaryChangeTask != nullptr) {
        rotaryChangeTask->forceNextIteration();
    }
    //String message = "Rotary encoder button pressed for: " + String(duration) + " ms";
    //logger.log(message.c_str());
}