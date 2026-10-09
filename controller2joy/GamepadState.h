#pragma once
#include <stdint.h>

enum : uint8_t { AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, AXIS_COUNT };

// Controller-independent snapshot produced by every USB driver.
struct GamepadState {
  bool connected = false;
  uint16_t vid = 0;
  uint16_t pid = 0;
  int8_t hatX = 0;  // -1 left, +1 right
  int8_t hatY = 0;  // -1 up,   +1 down
  uint8_t axis[AXIS_COUNT] = {128, 128, 128, 128};  // 0..255, 128 = centre, 0 = left/up
  uint32_t buttons = 0;  // bit n = HID button n+1
};

constexpr uint32_t btn(uint8_t hidButtonNumber) { return 1UL << (hidButtonNumber - 1); }
