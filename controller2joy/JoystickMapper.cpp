#include "JoystickMapper.h"
#include "config.h"

namespace {

int8_t stickDirection(uint8_t v) {
  if (v < 128 - STICK_THRESHOLD) return -1;
  if (v > 127 + STICK_THRESHOLD) return 1;
  return 0;
}

}  // namespace

JoyState mapGamepad(const GamepadState &pad, const ControllerProfile &profile, uint32_t nowMs) {
  JoyState js;
  if (!pad.connected) return js;

  int8_t sx = 0, sy = 0;
  if (profile.useLeftStick) {
    sx = stickDirection(pad.axis[AXIS_LX]);
    sy = stickDirection(pad.axis[AXIS_LY]);
  }

  js.left = pad.hatX < 0 || sx < 0;
  js.right = pad.hatX > 0 || sx > 0;
  js.up = pad.hatY < 0 || sy < 0 || (pad.buttons & profile.upMask);
  js.down = pad.hatY > 0 || sy > 0;

  // A real stick can never close opposite contacts; some games misbehave if it happens.
  if (js.left && js.right) js.left = js.right = false;
  if (js.up && js.down) js.up = js.down = false;

  const bool autofirePhase = ((nowMs / AUTOFIRE_HALF_PERIOD_MS) & 1) == 0;
  js.fire = (pad.buttons & profile.fireMask) ||
            ((pad.buttons & profile.autofireMask) && autofirePhase);
  return js;
}
