# Verified XIAO 3.3 V TrackPoint prototype

This wiring was verified on 2026-09-27 with the photographed
13S5561/T400-family module. Pointer movement works over USB and over Bluetooth
while the XIAO is battery powered.

The result applies to this module and does not establish that every visually
similar W500/R60-era module accepts 3.3 V. Do not increase the supply to 5 V
with this direct wiring in place.

## Connections

| TrackPoint contact | Assumed function | XIAO connection |
| ---: | --- | --- |
| 1 | RESET | `D9` / nRF52840 `P1.14`, preferably through 1 kOhm |
| 2 | VCC | `3V3` |
| 3 | PS/2 CLOCK | `D5` / nRF52840 `P0.05` |
| 4 | GND | `GND` |
| 5 | MB0 / left button | Leave unconnected |
| 6 | MB1 / right button | Leave unconnected |
| 7 | PS/2 DATA | `D4` / nRF52840 `P0.04` |
| 8 | MB2 / middle button | Leave unconnected |

The verified prototype has **no added CLOCK or DATA pull-up resistors**. It
works despite an unpowered resistance measurement of approximately 18 MOhm
from VCC contact 2 to CLOCK contact 3. The firmware does not deliberately
enable GPIO pull-ups, so the source and strength of the working idle-high bias
remain to be characterized. Preserve optional 4.7 kOhm pull-up footprints in a
future PCB, but do not alter the known-working prototype solely to add them.

Optional breadboard decoupling is a 100 nF ceramic capacitor between contacts
2 and 4 close to the module. A 10 uF capacitor may be added in parallel, with
positive to contact 2.

## Before applying power

1. Disconnect USB and any battery.
2. Confirm there is no low resistance between assumed VCC contact 2 and GND
   contact 4.
3. Confirm CLOCK, DATA, and RESET are not connected to 5 V.
4. Leave contacts 5, 6, and 8 isolated for this movement-only test.
5. Check for shorts, then power the XIAO from USB or its battery.

The firmware drives RESET high for approximately 600 ms and then low, matching
the direct-reset sequence used by the experimental driver. It configures DATA
as UARTE receive/GPIO, CLOCK as GPIO, and suppresses all button reports.

## Applicability to other modules

For a different module that does not respond at 3.3 V, stop and disconnect
power before rewiring. The next test is a 5 V TrackPoint supply with proper
open-drain level translation on CLOCK and DATA. RESET also needs translation
or the documented RC network; D9 must not remain directly connected to a 5
V-powered module. A BSS138-style two-channel I2C level-shifter can be used for
CLOCK and DATA, with pull-ups to 3.3 V on its low side and 5 V on its high side.

## Firmware pin assignment

- PS/2 DATA/UARTE RX: D4/P0.04
- PS/2 CLOCK: D5/P0.05
- TrackPoint RESET: D9/P1.14
- Receiver speed: 14,400 baud
- Physical buttons: ignored

The XIAO D4/D5 mappings come from the Seeed board definition used by ZMK v0.3.
The driver routes its unused UART TX signal to unexposed P0.27.

## Sources

- Seeed XIAO nRF52840 pin map:
  <https://wiki.seeedstudio.com/XIAO_BLE/>
- Candidate TrackPoint family map:
  <https://deskthority.net/wiki/TrackPoint_Hardware>
- Driver reset procedure:
  <https://github.com/infused-kim/kb_zmk_ps2_mouse_trackpoint_driver#343-trackpoint-power-on-reset>
