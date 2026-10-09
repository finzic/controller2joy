#pragma once
#include <stdint.h>
#include "JoystickMapper.h"

enum class PortMode : uint8_t {
  Atari,          // Kempston interface, C64, Atari, Amiga, MSX-style switch-to-GND
  SinclairPlus2,  // Amstrad ZX Spectrum +2 / +2A / +3 "Sinclair" ports
};

// Drives the DB-9 through a 74HC595 and optocouplers. 595 output Qn closes the
// switch on DB-9 pin n+1 (Q0..Q6 = pins 1..7); Q7 is the status LED.
class JoyPort {
 public:
  void begin();
  void write(const JoyState &js, PortMode mode, bool led);

 private:
  void shift(uint8_t value);
  int16_t last_ = -1;
};
