# TrackPoint research summary

Scope: information collected for the Zaphod TrackPoint project through
2026-09-21.

## Protocol conclusion

Classic IBM and Lenovo TrackPoints most commonly use PS/2. PS/2 is a
device-clocked, bidirectional, two-wire open-collector bus; it is not SPI or
UART. A frame contains a start bit, eight data bits least-significant-bit
first, odd parity, and a stop bit. The TrackPoint normally generates the
clock, including while receiving host commands.

Some newer pointing-stick assemblies use I2C, usually with SDA, SCL, and an
interrupt line. Their report format and voltage requirements are
module-specific. Connector appearance and conductor count do not identify the
protocol.

No authoritative TrackPoint-specific definition of "SCI" has been found.
Do not interpret an SCI label as SPI, UART, or a known TrackPoint bus without
documentation for that exact module.

## Salvaged W500/R60-family assembly

The purchased keyboard is compatible with ThinkPad W500/R60-family machines,
which makes a classic 5 V PS/2 TrackPoint plausible, but not proven. Lenovo
used multiple internal assemblies and connector layouts. The exact keyboard
FRU, TrackPoint PCB photographs, and continuity measurements are required
before applying power.

Expected signals for a TPM754-style assembly are VCC, GND, CLOCK, DATA, and
RESET. Buttons are separate controller inputs and are not required for the
first pointer-movement test.

## Electrical constraints

- The XIAO nRF52840 uses 3.3 V GPIO. Do not expose its pins to 5 V pull-ups.
- For a 5 V TrackPoint, power the prototype from USB VBUS and translate CLOCK
  and DATA with a bidirectional open-drain level shifter.
- Provide pull-ups on both voltage domains unless measurements show that the
  module already supplies the 5 V-side pull-ups.
- A typical starting value is 4.7 kOhm for each CLOCK and DATA pull-up.
- Add local supply decoupling near the TrackPoint during breadboard testing.
- The XIAO 5 V pin is USB VBUS and is not a 5 V output during battery-only
  operation. A final battery-powered design may require a boost converter.

## RC power-on reset

The TPM754 RESET input is active high and has no internal reset resistor. The
Philips reference schematic uses:

```text
+5 V --- 2.2 uF --- RESET
                       |
                     100 kOhm
                       |
                      GND
```

For a polarized capacitor, connect its positive terminal to +5 V. The nominal
time constant is 220 ms. With a 5 V supply and the documented RESET-high
threshold of 0.7 x VCC, the idealized pulse remains above that threshold for
about 78 ms. The controller itself requires RESET high for only two machine
cycles. The longer approximately 600 ms interval is the TrackPoint's
post-reset startup/self-test period, so firmware must still wait for and
validate the `AA 00` response.

This circuit saves a GPIO but only produces a power-on pulse. Quick power
cycles may fail if the capacitor has not discharged, and a slowly rising
future boost supply must be tested. A reset supervisor is the more robust
fallback if the RC behavior is unreliable.

## Standalone XIAO prototype

Provisional high-frequency-friendly assignments:

| Function | XIAO pin | nRF52840 pin |
| --- | --- | --- |
| PS/2 DATA | D4 | P0.04 |
| PS/2 CLOCK | D5 | P0.05 |
| TrackPoint RESET | none | 5 V-side RC circuit |
| TrackPoint power | 5V/VBUS | USB-powered prototype only |
| Common reference | GND | GND |

The XIAO test firmware should be a minimal standalone ZMK target rather than
the complete Zaphod Lite shield. Zaphod Lite already allocates nearly every
XIAO pin to its keyboard matrix, shift register, and display.

## ZMK integration direction

Current ZMK pointing support consumes Zephyr input events and emits HID mouse
reports through `zmk,input-listener`. The prototype should therefore contain:

- a PS/2 transport;
- a TrackPoint initialization and packet parser;
- relative X/Y Zephyr input events; and
- an upstream ZMK input listener.

It should not contain the old external module's custom listener, mouse-key
behaviors, movement-activated layer, or legacy HID integration. Physical
button events may be added later if desired.

The prior external PS/2 module remains useful technical evidence, especially
its nRF52 UARTE-assisted receiver. Its main branch predates ZMK v0.3, has
known current-ZMK integration problems, and has no repository-level license.
Retain the SPDX notice of any reused file and minimize copied code.

## Zaphod screen constraint

The Zaphod v1 display connector has six display signals, VCC, GND, and one
unconnected contact. The firmware currently claims P0.07 for display clock,
P0.05 for display data, and P0.04 for chip select. The schematic also routes
P0.12, P0.21, and P0.23 as display control signals.

Keeping both the screen and TrackPoint is possible only with hardware
rework/interposition or a board revision; it is not a plug-in firmware-only
change. That decision is intentionally deferred until the standalone XIAO
prototype proves the salvaged TrackPoint and BLE path.

## Power budget

Zaphod's XC6206 regulator is rated for a maximum of 200 mA. A future
integration must account for the nRF52840, radio peaks, display, LEDs,
TrackPoint, level translation, and any 5 V boost converter. Verify the exact
TrackPoint current instead of relying on a generic estimate.
