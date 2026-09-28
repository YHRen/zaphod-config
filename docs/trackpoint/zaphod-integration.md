# Zaphod TrackPoint integration

The Sharp memory display can remain connected and use its existing pins. The
Zaphod board leaves three adjacent edge pads on the Holyiot 18010 module
unconnected, so the TrackPoint can use those pads instead of sharing the
display's SPI signals.

## Signal plan

| Function | nRF52840 GPIO | Holyiot pad | Existing Zaphod use |
| --- | --- | ---: | --- |
| TrackPoint CLOCK | P1.11 | 2 | Unconnected |
| TrackPoint DATA / UARTE RX | P1.10 | 3 | Unconnected |
| TrackPoint RESET | P1.13 | 4 | Unconnected |
| TrackPoint VCC | VCC / 3.3 V | - | Display connector J1 pad 8 or another regulated VCC point |
| TrackPoint GND | GND | - | Display connector J1 pad 3 or another ground point |

The three signal pads sit next to one another on the module edge. This is more
practical for a hand-wired prototype than P1.12 and P1.14, which are module
underside pads.

The display continues to use:

| Display function | nRF52840 GPIO |
| --- | --- |
| SPI clock | P0.07 |
| SPI data | P0.05 |
| Chip select | P0.04 |
| EXTMODE | P0.12 |
| EXTCOMIN | P0.21 |
| DISP | P0.23 |

Do not attach the TrackPoint to P0.04 or P0.05 as on the XIAO prototype. Those
two pins are already connected to the display and cannot safely carry the
bidirectional PS/2 bus.

## Firmware target

The optional `zaphod_trackpoint` shield adds the proven UART-assisted PS/2
receiver to the existing Zaphod board without changing the base `zaphod`
target. It preserves the keyboard matrix, Sharp display, USB, and Bluetooth.
It keeps TrackPoint button reports disabled during the first integration.

GitHub Actions produces two Zaphod images:

- `zaphod` - existing keyboard and display firmware;
- `zaphod-trackpoint` - keyboard, display, and TrackPoint firmware.

## Prototype procedure

1. Solder fine insulated wires to Holyiot pads 2, 3, and 4. Verify each wire
   with continuity mode and verify there are no bridges to adjacent pads.
2. Connect the photographed TrackPoint contact 3 to CLOCK/P1.11, contact 7 to
   DATA/P1.10, and contact 1 to RESET/P1.13 through an optional 1 kOhm series
   resistor.
3. Connect TrackPoint contact 2 to the keyboard's regulated 3.3 V rail and
   contact 4 to ground. Leave contacts 5, 6, and 8 unconnected.
4. For the first test, power the keyboard over USB through a current meter and
   flash the `zaphod-trackpoint` artifact.
5. Verify the display and every key first, then test pointer movement over USB
   and Bluetooth.
6. Measure idle and sustained-movement current before relying on battery power.

The verified XIAO prototype worked without external CLOCK or DATA pull-ups,
but their source and strength have not been characterized. Keep optional
4.7 kOhm pull-up footprints in a future PCB revision. For this hand-wired test,
start by reproducing the known-working no-added-pull-up circuit.

## Risks to verify

- The XC6206 3.3 V regulator is rated for up to 200 mA, but the complete
  keyboard, display, radio, and TrackPoint current must be measured together.
- Confirm TrackPoint cold-start behavior as the LiPo voltage approaches the
  regulator's dropout region.
- The PS/2 receiver raises GPIOTE interrupt priority to meet the bus timing.
  Test sustained pointer movement while typing, refreshing the display, and
  using Bluetooth.
