#include "ControllerProfiles.h"
#include "GamepadState.h"

namespace {

constexpr uint32_t FACE_1_TO_4 = btn(1) | btn(2) | btn(3) | btn(4);
constexpr uint32_t SHOULDERS_5_TO_8 = btn(5) | btn(6) | btn(7) | btn(8);

const ControllerProfile PROFILES[] = {
    // Logitech F310/F710 in "D" mode report the same IDs as the Dual Action.
    {0x046D, 0xC216, "Logitech Dual Action / F310 (D)", FACE_1_TO_4, SHOULDERS_5_TO_8, 0, true},
    // 8BitDo D-input layout: B=1 A=2 Y=4 X=5 L=7 R=8 Select=11 Start=12.
    {0x2DC8, 0, "8BitDo (D-input mode)", btn(1) | btn(2) | btn(4) | btn(5), btn(7) | btn(8), 0, true},
    {0x0079, 0x0011, "DragonRise SNES-style pad", FACE_1_TO_4, btn(5) | btn(6), 0, true},
    // Also matches 8BitDo pads in X-input mode, which present as an Xbox 360 pad.
    {0x045E, 0x028E, "Xbox 360 / X-input pad", FACE_1_TO_4, SHOULDERS_5_TO_8, 0, true},
};

const ControllerProfile GENERIC = {0, 0, "Generic HID gamepad", FACE_1_TO_4, SHOULDERS_5_TO_8, 0, true};

}  // namespace

const ControllerProfile &findProfile(uint16_t vid, uint16_t pid) {
  const ControllerProfile *vendorMatch = nullptr;
  for (const ControllerProfile &p : PROFILES) {
    if (p.vid != vid) continue;
    if (p.pid == pid) return p;
    if (p.pid == 0 && !vendorMatch) vendorMatch = &p;
  }
  return vendorMatch ? *vendorMatch : GENERIC;
}
