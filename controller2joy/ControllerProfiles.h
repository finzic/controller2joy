#pragma once
#include <stdint.h>

// Button roles per controller. Masks use HID button numbering: btn(1) = button 1.
// XInput pads are translated to: 1=A 2=B 3=X 4=Y 5=LB 6=RB 7=LT 8=RT
// 9=Back 10=Start 11=L3 12=R3 13=Guide.
struct ControllerProfile {
  uint16_t vid;
  uint16_t pid;  // 0 = any product of this vendor
  const char *name;
  uint32_t fireMask;      // held = fire held
  uint32_t autofireMask;  // held = fire pulsing
  uint32_t upMask;        // held = "up" (jump in platformers); 0 = disabled
  bool useLeftStick;      // left stick drives the directions too
};

const ControllerProfile &findProfile(uint16_t vid, uint16_t pid);
