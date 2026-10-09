#include "JoyPort.h"
#include <Arduino.h>
#include <SPI.h>
#include "config.h"

namespace {

struct Db9Map {
  uint8_t up, down, left, right, fire;  // DB-9 pin numbers
};

// Atari/Kempston/C64: 1 up, 2 down, 3 left, 4 right, 6 fire, 7 +5V, 8 GND.
constexpr Db9Map ATARI_MAP = {1, 2, 3, 4, 6};
// Spectrum +2/+2A/+3: 3 right, 4 left, 5 down, 6 up, 7 fire, 8 common.
constexpr Db9Map PLUS2_MAP = {6, 5, 4, 3, 7};

constexpr uint8_t LED_BIT = 0x80;

constexpr uint8_t pinBit(uint8_t db9Pin) { return 1u << (db9Pin - 1); }

}  // namespace

void JoyPort::begin() {
  digitalWrite(PIN_SR_OE, HIGH);  // set level before enabling the driver: no glitch
  pinMode(PIN_SR_OE, OUTPUT);
  digitalWrite(PIN_SR_LATCH, LOW);
  pinMode(PIN_SR_LATCH, OUTPUT);
  SPI.begin();
  shift(0);
  last_ = 0;
  digitalWrite(PIN_SR_OE, LOW);
}

void JoyPort::write(const JoyState &js, PortMode mode, bool led) {
  const Db9Map &m = (mode == PortMode::SinclairPlus2) ? PLUS2_MAP : ATARI_MAP;
  uint8_t v = led ? LED_BIT : 0;
  if (js.up) v |= pinBit(m.up);
  if (js.down) v |= pinBit(m.down);
  if (js.left) v |= pinBit(m.left);
  if (js.right) v |= pinBit(m.right);
  if (js.fire) v |= pinBit(m.fire);
  if (v == last_) return;
  shift(v);
  last_ = v;
}

void JoyPort::shift(uint8_t value) {
  // Traffic to the MAX3421E also clocks the 595's shift register, but the
  // outputs only change on the latch edge below.
  SPI.beginTransaction(SPISettings(SR_SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
  SPI.transfer(value);
  SPI.endTransaction();
  digitalWrite(PIN_SR_LATCH, HIGH);
  digitalWrite(PIN_SR_LATCH, LOW);
}
