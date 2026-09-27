# TrackPoint integration notes

This folder collects the hardware research, source documents, figures, and
bring-up decisions for adding a salvaged ThinkPad TrackPoint to Zaphod.

## Current direction

- Preserve the verified standalone prototype on a Seeed Studio XIAO nRF52840
  before modifying the Zaphod keyboard.
- The photographed W500/R60-era assembly operates from 3.3 V using the
  assumed eight-contact PS/2 pinout.
- Start with pointer movement only. Do not add ZMK mouse-key behaviors,
  automatic mouse layers, or keyboard-emulated buttons.
- Use ZMK's upstream input listener for HID reporting. Adapt only the PS/2
  transport and packet parser from prior work.
- Prefer the nRF52 UART-assisted receive technique over a GPIO-only receiver.
- Drive RESET from XIAO D9/P1.14 using the driver's 600 ms startup sequence.

## Verified prototype

Tested on 2026-09-27:

- TrackPoint VCC at 3.3 V.
- DATA on XIAO D4/P0.04 and CLOCK on D5/P0.05.
- RESET on D9/P1.14.
- No external CLOCK or DATA pull-up resistors installed.
- Cursor movement works over USB.
- Cursor movement works over Bluetooth while the XIAO is battery powered.
- No mouse-key behaviors or physical-button reports are enabled.

The source of the working bus's idle-high bias has not yet been isolated. The
firmware does not deliberately enable internal pull-ups, so a future PCB should
retain optional pull-up footprints even though they are not populated in the
verified prototype.

## Contents

- [research-summary.md](research-summary.md) - consolidated findings and
  design constraints.
- [bring-up-plan.md](bring-up-plan.md) - staged prototype and verification
  procedure.
- [module-identification.md](module-identification.md) - visual identification,
  evidence quality, and the eight-contact map later validated by operation.
- [continuity-check.md](continuity-check.md) and
  [continuity-measurements.csv](continuity-measurements.csv) - the passive
  multimeter checks required before power-up.
- [xiao-wiring.md](xiao-wiring.md) - exact USB-powered XIAO connection table
  and electrical constraints for the assumed eight-contact pinout.
- [figures/xiao-13s5561-wiring.svg](figures/xiao-13s5561-wiring.svg) - earlier
  conservative 5 V/level-shifter proposal, retained as a design reference but
  not used by the verified 3.3 V prototype.
- [figures/xiao-trackpoint-prototype.svg](figures/xiao-trackpoint-prototype.svg)
  - earlier logical 5 V prototype proposal, not the verified wiring.
- [figures/13s5561-connector-candidate.svg](figures/13s5561-connector-candidate.svg)
  - connector orientation and signals later validated by operation.
- `photos/` - original submitted front/back module photographs.
- [sources/README.md](sources/README.md) - source provenance and external
  references.
- `sources/zaphod-v1-schematic.pdf` - original Zaphod v1 schematic.
- `sources/tpm754-datasheet.pdf` - Philips TPM754 TrackPoint-controller
  datasheet.
- `sources/figures/tpm754-reference-schematic.png` - reference TrackPoint
  circuit containing the RC reset network.

## Remaining characterization

1. Record the keyboard FRU and, if possible, take a glare-free macro photo of
   the main IC markings.
2. Measure powered idle voltage and, if possible, effective pull-up strength on
   CLOCK and DATA.
3. Measure TrackPoint current during idle and sustained movement.
4. Run longer BLE stability and cold-start tests before migrating to Zaphod.

Last consolidated: 2026-09-27.
