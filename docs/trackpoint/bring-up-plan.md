# XIAO TrackPoint bring-up plan

Goal: demonstrate reliable pointer movement from the salvaged TrackPoint over
USB and then Bluetooth using ZMK, without mouse-key behaviors or a keyboard
matrix.

## Equipment and parts

- Seeed Studio XIAO nRF52840.
- Salvaged ThinkPad TrackPoint assembly.
- Two 4.7 kOhm pull-up resistors for the first 3.3 V direct test.
- One 1 kOhm series resistor for the D9 RESET connection.
- Two-channel bidirectional open-drain level shifter for a later 5 V test.
- Supply decoupling, initially 100 nF plus 10 uF near the TrackPoint.
- Breadboard, fine wire, multimeter, and preferably a logic analyzer.
- USB connection for XIAO power, flashing, and logs.

## Stage 0: identify the assembly

Do not power the module during this stage.

1. Record the keyboard FRU and connector orientation.
2. Photograph both sides of the TrackPoint PCB at readable resolution.
3. Trace ground and supply planes with a multimeter.
4. Locate controller pins or known test pads for CLOCK, DATA, and RESET.
5. Measure resistance from CLOCK and DATA to VCC to detect onboard pull-ups.
6. Check whether RESET already has a capacitor/resistor network.

For the photographed `13S5561 REV.B` module, use
[module-identification.md](module-identification.md) and complete
[continuity-check.md](continuity-check.md). The current prototype deliberately
assumes the visually matching eight-contact map, but that remains an assumption
until continuity or successful signaling confirms it.

Exit criterion: VCC, GND, CLOCK, DATA, and RESET are identified with evidence,
not only a visually similar online pinout.

## Stage 1: safe electrical bring-up

1. Assemble the 3.3 V direct circuit in [xiao-wiring.md](xiao-wiring.md).
2. Add separate 4.7 kOhm pull-ups from CLOCK and DATA to 3.3 V. The measured
   18 MOhm from VCC to CLOCK does not provide the required pull-up.
3. Connect RESET to D9/P1.14, preferably through a 1 kOhm series resistor.
4. Power the TrackPoint from XIAO 3V3 for this first, current-limited test.
5. Verify the 3.3 V rail before connecting the TrackPoint.
6. Verify that CLOCK, DATA, and RESET cannot be exposed to 5 V.

Exit criterion: the power rails and idle signal levels are correct and no
device overheats or draws unexpected current.

## Stage 2: observe TrackPoint startup

1. Power-cycle with sufficient off time for the RESET capacitor to discharge.
2. Capture RESET, CLOCK, and DATA with a logic analyzer.
3. Confirm RESET stays high for about 600 ms and then returns to ground.
4. Confirm activity after the TrackPoint startup interval.
5. Record the measured PS/2 clock period for UART baud selection.

Exit criterion: repeatable startup and plausible PS/2 signaling.

## Stage 3: minimal ZMK transport

Create a dedicated XIAO test target with provisional assignments:

- D4/P0.04: PS/2 DATA and UARTE receive input;
- D5/P0.05: PS/2 CLOCK;
- USB CDC ACM: logs;
- D9/P1.14: active-high startup RESET pulse;
- no matrix, display, split transport, or mouse-key behavior.

Start with 14,400 baud for the nRF52 UART-assisted receiver. If initialization
fails, compare the measured TrackPoint clock frequency and try the closest
supported receiver rate. A GPIO receiver may be used temporarily to validate
pinout and device health, but it is not the preferred BLE implementation.

Exit criterion: logs show successful initialization, including `AA 00`, and
valid three-byte movement packets.

The initial test firmware is built with `./build-trackpoint-test.sh`. It uses
the upstream `zmk,input-listener`, ignores TrackPoint button bits, and contains
no mouse-key behaviors.

## Stage 4: USB HID pointer

1. Emit only relative X and Y input events.
2. Connect them to an upstream `zmk,input-listener`.
3. Test direction, sign, packet synchronization, and sustained movement over
   USB.

Exit criterion: smooth USB cursor movement without packet loss or unintended
button events.

## Stage 5: Bluetooth HID pointer

1. Enable the normal ZMK BLE endpoint.
2. Pair with a test host and verify the pointer report descriptor.
3. Compare motion quality with USB.
4. Run sustained movement while monitoring connection stability and logs.

Exit criterion: repeatable pairing and smooth BLE cursor movement without
disconnects or obvious radio interference.

## Stage 6: migrate to Zaphod

Only after the standalone prototype passes:

1. Measure TrackPoint current and estimate battery impact.
2. Select the Zaphod signal-routing strategy while preserving the display.
3. Design an interposer or board revision with proper 5 V generation and
   level translation.
4. Move the proven transport/parser into the Zaphod build.
5. Re-test display operation, BLE range, and power consumption together.
