#pragma once
#include <Arduino.h>
#include "TaskSchedulerDeclarations.h"
#include <ESP32RotaryEncoder.h>

class EncoderInput
{
private:
    RotaryEncoder rotaryEncoder;
    void knobCallback( long value );
    void buttonCallback( unsigned long duration );
public:
    Task *rotaryChangeTask = nullptr;

    // Constructor to initialize rotaryEncoder
    EncoderInput() : rotaryEncoder(16, 17, 18) {}

    void setup();
};
