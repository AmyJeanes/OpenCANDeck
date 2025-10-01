// Centralized hardware-related constants for the OpenCANDeck input deck.
#pragma once

#include <stdint.h>

// I2C addresses (default Adafruit breakout values)
constexpr uint8_t NEOKEY_I2C_ADDR   = 0x30; // NeoKey 1x4
constexpr uint8_t ENCODER_I2C_ADDR  = 0x36; // Rotary encoder w/ NeoPixel (seesaw)

// Encoder seesaw pin assignments
constexpr uint8_t ENCODER_SWITCH_PIN = 24;  // GPIO for push switch (active low)
constexpr uint8_t ENCODER_PIXEL_PIN  = 6;   // NeoPixel data pin on encoder board

// Pixel brightness levels
constexpr uint8_t ENCODER_PIXEL_BRIGHTNESS = 20; // range 0-255

// Timing
constexpr uint16_t INPUT_POLL_DELAY_MS = 10; // base loop delay
