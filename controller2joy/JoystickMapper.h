#pragma once
#include <stdint.h>
#include "ControllerProfiles.h"
#include "GamepadState.h"

struct JoyState {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  bool fire = false;
};

JoyState mapGamepad(const GamepadState &pad, const ControllerProfile &profile, uint32_t nowMs);
