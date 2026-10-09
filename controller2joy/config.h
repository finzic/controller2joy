#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// Pin assignment (GPIO numbers).
// The MAX3421E SS/INT pins are fixed by the USB Host Shield library (UsbCore.h):
//   ESP8266: SS = GPIO15 (D8), INT = GPIO5 (D1), SPI SCK/MISO/MOSI = D5/D6/D7
//   ESP32  : SS = GPIO5,       INT = GPIO17,     SPI SCK/MISO/MOSI = 18/19/23
// The 74HC595 output register shares SCK/MOSI with the MAX3421E.
// ---------------------------------------------------------------------------
#if defined(ESP8266)
constexpr uint8_t PIN_SR_LATCH = 4;   // D2 -> 74HC595 RCLK (pin 12)
constexpr uint8_t PIN_SR_OE    = 2;   // D4 -> 74HC595 /OE (pin 13); GPIO2 is high at boot = outputs off
constexpr uint8_t PIN_MODE     = 16;  // D0 <- mode switch; needs external 10k pull-up (GPIO16 has none)
constexpr bool    MODE_PIN_INTERNAL_PULLUP = false;
#elif defined(ESP32)
constexpr uint8_t PIN_SR_LATCH = 21;
constexpr uint8_t PIN_SR_OE    = 22;  // add 10k pull-up to 3V3 so outputs stay off during boot
constexpr uint8_t PIN_MODE     = 27;
constexpr bool    MODE_PIN_INTERNAL_PULLUP = true;
#elif defined(ARDUINO)
#error "Unsupported board: add a pin assignment to config.h"
#endif

// ---------------------------------------------------------------------------
// Behaviour
// ---------------------------------------------------------------------------
// Left analogue stick also drives the directions (needed for pads whose D-pad
// is reported as X/Y axes, and for the Logitech "Mode" button that swaps them).
constexpr uint8_t  STICK_THRESHOLD         = 64;   // distance from centre (0..127)
constexpr uint16_t AUTOFIRE_HALF_PERIOD_MS = 60;   // ~8 shots per second
constexpr uint16_t MODE_DEBOUNCE_MS        = 30;
constexpr uint32_t SR_SPI_CLOCK_HZ         = 8000000;

// Print decoded controller state on every change (useful to build new profiles).
#define DEBUG_PAD 0
