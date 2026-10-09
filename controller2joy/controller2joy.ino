// controller2joy - USB gamepad to DB-9 digital joystick adapter
// Targets: ESP8266 (Wemos D1 mini) or ESP32 + MAX3421E USB host board.

#include <SPI.h>
#include <usbhub.h>
#include <XBOXUSB.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "config.h"
#include "ControllerProfiles.h"
#include "HidGamepad.h"
#include "JoyPort.h"
#include "JoystickMapper.h"

USB Usb;
USBHub Hub(&Usb);
HidGamepad Hid(&Usb);
XBOXUSB Xbox(&Usb);
JoyPort Port;

static bool usbOk = false;

static PortMode readMode(uint32_t now) {
  static bool stableRaw = true;
  static bool lastRaw = true;
  static uint32_t changedAt = 0;
  const bool raw = digitalRead(PIN_MODE);
  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  } else if (raw != stableRaw && now - changedAt >= MODE_DEBOUNCE_MS) {
    stableRaw = raw;
  }
  // Switch closed to GND = Spectrum +2.
  return stableRaw ? PortMode::Atari : PortMode::SinclairPlus2;
}

static uint8_t xinputAxis(int16_t v) { return static_cast<uint8_t>((static_cast<int32_t>(v) + 32768) >> 8); }

static void readXbox(GamepadState &pad) {
  pad.connected = true;
  pad.vid = 0x045E;
  pad.pid = 0x028E;
  pad.hatX = (Xbox.getButtonPress(RIGHT) ? 1 : 0) - (Xbox.getButtonPress(LEFT) ? 1 : 0);
  pad.hatY = (Xbox.getButtonPress(DOWN) ? 1 : 0) - (Xbox.getButtonPress(UP) ? 1 : 0);
  // XInput Y axes are positive when pushed up.
  pad.axis[AXIS_LX] = xinputAxis(Xbox.getAnalogHat(LeftHatX));
  pad.axis[AXIS_LY] = 255 - xinputAxis(Xbox.getAnalogHat(LeftHatY));
  pad.axis[AXIS_RX] = xinputAxis(Xbox.getAnalogHat(RightHatX));
  pad.axis[AXIS_RY] = 255 - xinputAxis(Xbox.getAnalogHat(RightHatY));

  // Order defines the button numbering documented in ControllerProfiles.h.
  static const ButtonEnum order[] = {A, B, X, Y, L1, R1, L2, R2, BACK, START, L3, R3, XBOX};
  pad.buttons = 0;
  for (uint8_t i = 0; i < sizeof(order) / sizeof(order[0]); ++i) {
    const ButtonEnum b = order[i];
    const uint8_t v = Xbox.getButtonPress(b);
    const bool pressed = (b == L2 || b == R2) ? v > 64 : v != 0;  // triggers are analogue
    if (pressed) pad.buttons |= 1UL << i;
  }
}

static void logChanges(const GamepadState &pad, const ControllerProfile &profile, PortMode mode) {
  static bool wasConnected = false;
  static bool wasUnsupported = false;
  static int8_t lastMode = -1;

  if (static_cast<int8_t>(mode) != lastMode) {
    lastMode = static_cast<int8_t>(mode);
    Serial.println(mode == PortMode::Atari ? F("Mode: Kempston / C64 / Atari")
                                           : F("Mode: ZX Spectrum +2/+2A/+3"));
  }
  if (pad.connected != wasConnected) {
    wasConnected = pad.connected;
    if (pad.connected) {
      Serial.printf("Connected %04X:%04X -> profile \"%s\"\n", pad.vid, pad.pid, profile.name);
      if (!Xbox.Xbox360Connected) {
        const GamepadLayout &l = Hid.layout();
        Serial.printf("  hat:%s X/Y:%s buttons:%u reportIds:%s\n", l.hat.valid ? "yes" : "no",
                      l.axis[AXIS_LX].valid ? "yes" : "no", l.buttonCount,
                      l.usesReportIds ? "yes" : "no");
      }
    } else {
      Serial.println(F("Controller disconnected"));
    }
  }
  const bool unsupported = Hid.isReady() && !Hid.state().connected;
  if (unsupported != wasUnsupported) {
    wasUnsupported = unsupported;
    if (unsupported) Serial.println(F("HID device attached but no gamepad controls found"));
  }

#if DEBUG_PAD
  static GamepadState last;
  if (memcmp(&last, &pad, sizeof(pad)) != 0) {
    last = pad;
    Serial.printf("hat %+d,%+d  L %3u,%3u  R %3u,%3u  buttons %08lX\n", pad.hatX, pad.hatY,
                  pad.axis[AXIS_LX], pad.axis[AXIS_LY], pad.axis[AXIS_RX], pad.axis[AXIS_RY],
                  static_cast<unsigned long>(pad.buttons));
  }
#endif
}

void setup() {
  Port.begin();  // first: force all joystick lines released
  pinMode(PIN_MODE, MODE_PIN_INTERNAL_PULLUP ? INPUT_PULLUP : INPUT);

  Serial.begin(115200);
#if defined(ESP8266)
  // Radio is unused; switching it off saves ~70 mA.
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
#endif

  usbOk = Usb.Init() != -1;
  Serial.println(usbOk ? F("controller2joy ready") : F("MAX3421E not responding - check wiring"));
}

void loop() {
  Usb.Task();
  const uint32_t now = millis();
  const PortMode mode = readMode(now);

  GamepadState pad;
  if (Xbox.Xbox360Connected) readXbox(pad);
  else pad = Hid.state();

  const ControllerProfile &profile = findProfile(pad.vid, pad.pid);
  logChanges(pad, profile, mode);

  const bool led = usbOk ? pad.connected : ((now / 125) & 1);  // fast blink = USB host fault
  Port.write(mapGamepad(pad, profile, now), mode, led);
}
