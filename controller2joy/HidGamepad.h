#pragma once
#include <hiduniversal.h>
#include "GamepadState.h"
#include "HidReportDescriptor.h"

// Generic DirectInput-style USB HID gamepad (Logitech Dual Action, 8BitDo in
// D-input mode, SNES-style pads, ...). The report layout is learned from the
// device's HID report descriptor, so no per-model decoding is needed.
class HidGamepad : public HIDUniversal {
 public:
  explicit HidGamepad(USB *usb) : HIDUniversal(usb) {}

  uint8_t Release() override;

  const GamepadState &state() const { return state_; }
  const GamepadLayout &layout() const { return layout_; }

 protected:
  uint8_t OnInitSuccessful() override;
  void ParseHIDData(USBHID *hid, bool isRptId, uint8_t len, uint8_t *buf) override;

 private:
  GamepadState state_;
  GamepadLayout layout_;
};
