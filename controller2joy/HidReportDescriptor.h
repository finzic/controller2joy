#pragma once
#include <stdint.h>
#include "GamepadState.h"

// Minimal HID report-descriptor parser: locates the hat switch, X/Y/Z/Rx/Ry/Rz
// axes and buttons of a generic (DirectInput-style) gamepad.

struct HidField {
  bool valid = false;
  uint8_t reportId = 0;
  uint16_t bitOffset = 0;  // offset after the report-ID byte
  uint8_t bitSize = 0;
  int32_t logicalMin = 0;
  int32_t logicalMax = 0;
};

constexpr uint8_t HID_MAX_BUTTONS = 32;

struct GamepadLayout {
  bool usesReportIds = false;
  HidField axis[AXIS_COUNT];
  HidField hat;
  HidField button[HID_MAX_BUTTONS];
  uint8_t buttonCount = 0;

  bool usable() const { return hat.valid || axis[AXIS_LX].valid || buttonCount > 0; }
};

bool parseHidReportDescriptor(const uint8_t *desc, uint16_t len, GamepadLayout &layout);

// `report` is the raw interrupt-IN payload (including the report-ID byte if used).
void decodeHidReport(const GamepadLayout &layout, const uint8_t *report, uint16_t len,
                     GamepadState &state);
