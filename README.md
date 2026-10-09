# controller2joy

Turns a modern USB gamepad (Logitech Dual Action, 8BitDo, Xbox 360-style, generic
HID pads) into a classic 9-pin digital joystick for:

- **Kempston / Atari standard** – ZX Spectrum with a Kempston interface, Commodore 64/VIC-20,
  Atari 8-bit/ST, Amiga, Amstrad CPC, MSX (digital part)
- **ZX Spectrum +2 / +2A / +3** – the Amstrad "Sinclair" joystick ports, which use a different
  pinout *and* a different electrical principle

The D-pad (and optionally the left stick) gives the directions, face buttons give fire,
shoulder buttons give autofire. A hardware switch selects the port mode.

The feasibility of an Acorn BBC Model B analogue output is evaluated in
[docs/bbc-micro-analogue.md](docs/bbc-micro-analogue.md).

## Why a MAX3421E is needed

The ESP8266 has **no USB host controller** (it has no USB peripheral at all), so it cannot talk
to a gamepad directly. The project uses a MAX3421E USB host chip over SPI (the cheap
"Mini USB Host Shield" boards), driven by the
[USB Host Shield 2.0](https://github.com/felis/USB_Host_Shield_2.0) library. The same code
builds for ESP32. (An ESP32-S2/S3 or RP2040 with native USB host could drop the MAX3421E, but
would need a different USB stack.)

## Bill of materials

| Qty | Part | Notes |
|---|---|---|
| 1 | Wemos D1 mini (ESP8266) or ESP32 DevKit | |
| 1 | MAX3421E "Mini USB Host Shield" | 3.3 V logic. **Cut the VBUS trace and feed VBUS with 5 V** – by default the board powers the USB socket from 3.3 V |
| 1 | 74HC595 | Output register sharing the SPI bus |
| 2 | PC847 (quad) or 7× PC817 optocouplers | Act as the joystick "switches" |
| 7 | 330 Ω | Opto LED resistors (~5 mA) |
| 1 | DPDT toggle switch | Mode select (Kempston/C64 ↔ +2) |
| 2 | 10 kΩ | Pull-ups for mode input (D0) and 595 `/OE` (D4) |
| 1 | LED + 330 Ω | Status (595 Q7) |
| 1 | DB-9 **female** cable/connector | The computer has the male plug |
| 1 | 5 V USB power supply | See *Power* |

## How the output works

Every retro joystick is just switches that connect a signal line to a common line. The adapter
reproduces that with optocouplers: the transistor side of each opto behaves like a switch, so
the same circuit works whatever the computer does with the lines.

That matters for the +2: there, **pin 8 is not ground** but a keyboard row line that the ROM
strobes low while scanning the keyboard, and the direction lines are keyboard columns
(the ports read as number keys 1–5 / 6–0). A plain "transistor to GND" output would press the
keys on *every* keyboard row; an opto between the signal pin and pin 8 behaves exactly like the
original joystick. As a bonus the adapter is galvanically isolated from the computer.

### DB-9 pinouts

| DB-9 pin | Kempston / C64 / Atari | Spectrum +2 / +2A / +3 |
|---|---|---|
| 1 | Up | not connected |
| 2 | Down | not connected |
| 3 | Left | Right |
| 4 | Right | Left |
| 5 | (POT Y / paddle) | Down |
| 6 | Fire | Up |
| 7 | **+5 V** | Fire |
| 8 | GND | Common (keyboard row strobe) |
| 9 | (POT X / paddle) | not connected |

> Check the +2 column against your machine's manual before connecting. The firmware tables
> are in [controller2joy/JoyPort.cpp](controller2joy/JoyPort.cpp).

In Kempston/C64 mode the firmware only ever closes pins 1, 2, 3, 4, 6; in +2 mode only 3, 4, 5,
6, 7. Because pin 7 is **+5 V** on Atari-standard ports, the pin-7 opto is additionally routed
through the mode switch, so a firmware bug can never short the computer's +5 V.

### Schematic

```
            ESP8266 (D1 mini)            MAX3421E mini host board
            D5 SCK  ─────────┬────────── SCK
            D7 MOSI ─────────│──┬─────── MOSI
            D6 MISO ─────────│──│─────── MISO
            D8 GPIO15 ───────│──│─────── SS
            D1 GPIO5  ───────│──│─────── INT
            3V3 ─────────────│──│─────── 3V3, RST (via 10k)
            5V  ─────────────│──│─────── VBUS (after cutting the 3V3→VBUS trace)
                             │  │
                    74HC595  │  │
            SRCLK (11) ──────┘  │
            SER   (14) ─────────┘
            RCLK  (12) ───────── D2 GPIO4
            /OE   (13) ───────── D4 GPIO2  (10k pull-up to 3V3)
            /SRCLR(10) ───────── 3V3
            VCC (16) 3V3, GND (8) GND

  One channel per DB-9 pin n = 1..7 (595 output Q[n-1]):

     Q[n-1] ──[330R]──►|── opto LED ──┐          opto transistor
                                     GND        C ──────────── DB-9 pin n
                                                E ──────────── DB-9 pin 8

  Channel 7 (pin 7) only:  C ──── switch pole A (closed in "+2") ──── DB-9 pin 7

  Mode switch pole B:      D0 GPIO16 ──┬── 10k ── 3V3
                                       └── pole B ── GND   (closed in "+2")

  Q7 ──[330R]── status LED ── GND
```

ESP32 pins: MAX3421E SS 5, INT 17, SPI 18/19/23; 595 latch 21, `/OE` 22; mode 27
(see [controller2joy/config.h](controller2joy/config.h)).

### Power

Power the adapter from an external 5 V USB supply. Do not take power from the joystick port:
the +2 port has no +5 V, and the C64 control-port supply is not meant for a gamepad plus
microcontroller. Thanks to the optocouplers no ground connection to the computer is needed.
On the ESP8266 the firmware switches the Wi-Fi radio off.

## Status LED

| LED | Meaning |
|---|---|
| off | no supported controller |
| on | controller connected |
| fast blink | MAX3421E not responding (wiring) |

## Supported controllers

| Controller | Notes |
|---|---|
| Logitech Dual Action (046D:C216) | Also F310/F710 in "D" mode. The *Mode* button swaps D-pad and left stick – both work |
| 8BitDo (VID 2DC8) in **D-input** mode | Generic HID driver |
| 8BitDo in **X-input** mode | Presents as an Xbox 360 pad (045E:028E) |
| Xbox 360 wired and compatible XInput pads | Via the library's `XBOXUSB` driver |
| Any USB HID gamepad/joystick | Report layout is read from the HID report descriptor (hat switch, X/Y/Z/Rz, 32 buttons) |

Not supported: Xbox One/Series pads, Switch Pro mode, PlayStation pads (the USB Host Shield
library has drivers for these, they could be added similarly to `XBOXUSB`), and Bluetooth.

### Button mapping

Default for all controllers: **buttons 1–4 = fire, buttons 5–8 (shoulders) = autofire**
(~8 Hz). Opposite directions pressed at the same time are suppressed.

Per-controller overrides live in
[controller2joy/ControllerProfiles.cpp](controller2joy/ControllerProfiles.cpp). Each profile sets
the fire, autofire and "up" (jump) button masks and whether the left stick also steers. To add a
controller:

1. Set `DEBUG_PAD` to `1` in [controller2joy/config.h](controller2joy/config.h) and open the serial
   monitor (115200 baud).
2. Plug in the pad: its VID:PID and detected layout are printed, then every button press.
3. Add a line to `PROFILES[]` using `btn(n)` for HID button `n`.

## Building

PlatformIO:

```sh
pio run -e d1_mini -t upload     # ESP8266
pio run -e esp32dev -t upload    # ESP32
pio device monitor
```

Arduino IDE: open `controller2joy/controller2joy.ino`, install **USB Host Shield Library 2.0**
from the Library Manager, select the board and upload.

## Usage

1. Set the mode switch **before** plugging into the computer.
2. ZX Spectrum (48K/128K) with Kempston interface, C64 etc.: *Kempston/C64* position.
3. Spectrum +2/+2A/+3: *+2* position. Each port acts as a set of number keys (Sinclair 1 =
   keys 6–0, Sinclair 2 = keys 1–5), so choose the matching "Sinclair"/"Interface 2" option in
   the game.

## License note

The USB Host Shield 2.0 library is GPL-2.0, so firmware binaries built with it are subject to
the GPL-2.0.
# controller2joy
