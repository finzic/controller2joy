# Feasibility: Acorn BBC Model B analogue joystick output

**Verdict: feasible, low risk, little extra hardware.** A dual digital potentiometer can
emulate the BBC's potentiometer joysticks exactly, and the firmware already has normalised
analogue stick values (`GamepadState::axis[]`, 0–255) ready to use.

## The BBC analogue port

15-pin D-sub (female on the computer), fed by a µPD7002 4-channel ADC.

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| 1 | +5 V | 9 | LPSTB (light pen) |
| 2 | 0 V | 10 | PB1 (fire, joystick 2) |
| 3 | 0 V | 11 | VREF |
| 4 | CH3 (joystick 2 Y) | 12 | CH2 (joystick 2 X) |
| 5 | 0 V | 13 | PB0 (fire, joystick 1) |
| 6 | 0 V | 14 | VREF |
| 7 | CH1 (joystick 1 Y) | 15 | CH0 (joystick 1 X) |
| 8 | 0 V | | |

> Verify against the *BBC Microcomputer User Guide* before building.

How an original BBC joystick works:

- Each axis is a potentiometer between **VREF (≈1.8 V)** and 0 V; the wiper goes to a CH input.
  The ADC measures the wiper voltage **relative to VREF**, so the result is ratiometric.
- Fire buttons are switches from PB0/PB1 to 0 V (inputs pulled up inside the BBC).
- Software reads `ADVAL(1)`…`ADVAL(4)` (0–65520) and the fire buttons with `ADVAL(0) AND 3`.
  On the BBC, left/up give *high* readings – check direction with `ADVAL` and invert in
  firmware if necessary.

## Emulation options

| Option | Accuracy | Parts | Comment |
|---|---|---|---|
| **Dual digital potentiometer** (MCP4231-103 7-bit or MCP4261-103 8-bit, 10 kΩ, SPI) | Exact – ratiometric to VREF | 1 IC per stick | **Recommended**. Terminal A → VREF, B → 0 V, wiper → CH. Behaves like the original pot |
| PWM + RC filter scaled to 0–1.8 V | Approximate | R, C | Not ratiometric, ripple, slow settling; drifts if VREF differs from the assumed value |
| DAC (MCP4822, classic ESP32 internal DAC) + divider | Good | DAC + resistors | Needs VREF tracking or calibration; ESP32-S2/S3 and ESP8266 have no suitable DAC |

Why the digital pot fits:

- VREF (1.8 V) is below the pot's 3.3 V supply, so all terminal voltages are in range.
- 7 bits (128 steps) is plenty: the µPD7002 delivers about 10 useful bits and most games
  use far fewer.
- MCP4xxx wipers power up at mid-scale, so the sticks read "centred" from power-on.
- Same SPI bus as the MAX3421E/74HC595; one extra chip select.

## Proposed hardware addition

```
   BBC VREF ──[1k]──┬── P0A ── P1A          MCP4231-103 (VDD 3V3)
                    │                        CS  ── D3 (GPIO0, high at boot = deselected)
   BBC 0 V  ────────┴── P0B ── P1B           SCK/MOSI shared with SPI bus
   BBC CH0  ─────────── P0W (X)              SDO not needed
   BBC CH1  ─────────── P1W (Y)

   2nd 74HC595 daisy-chained (QH' of the first → SER of the second):
     two extra optos: C → PB0 / PB1, E → BBC 0 V
```

- The 1 kΩ series resistor in VREF limits current into the pot's protection diodes if the BBC
  is switched on while the adapter is unpowered.
- The analogue side **must share 0 V with the BBC** (unlike the opto-isolated DB-9 side).
  Power the adapter from its own 5 V supply and connect the grounds; the BBC's +5 V pin is
  best left unused.
- A second MCP4231 (or one MCP4261 per stick pair) would add joystick 2 on CH2/CH3.

## Firmware effort

Small. A `BbcPort` class would:

1. Map `axis[AXIS_LX]`/`axis[AXIS_LY]` (0–255) to wiper positions 0–128 (with inversion as
   needed); optionally map the D-pad to full deflection for games written for digital-feeling
   sticks.
2. Write both wipers over SPI only when they change (the ADC needs ~10 ms per channel anyway).
3. Drive PB0/PB1 from the fire buttons via the second 595.

The DB-9 and DA-15 outputs can be driven at the same time, so no extra mode-switch position is
needed. Free pins remain on the ESP8266 (D3 for chip select; the second 595 needs no extra pins).

## Risks and limits

- Gamepad sticks have short travel and a deadzone, so precision is lower than with the
  original long-throw BBC joysticks; a configurable response curve can help.
- Pinout and ADVAL direction must be validated on real hardware.
- Games that use the analogue port for something else (e.g. light pen, paddles) need their own
  mappings.
