# Display-replacement firmware

Implemented in the `YHRen/trackpoint-display-replacement` worktree on
2026-09-30. The new target builds and passes software checks. It has **not
been flashed or tested on the Zaphod hardware**. The original
[replacement plan](display-replacement-plan.md) remains the hardware design
record; this document supersedes its proposed filenames and build commands.

## Target and wiring

Use board `zaphod`, shield `zaphod_trackpoint_display_replacement`, artifact
`zaphod-trackpoint-display-replacement`. Remove the entire Sharp display
connection before attaching the TrackPoint. This image and the spare-pad
`zaphod_trackpoint` shield are mutually exclusive; both overlay orders are
rejected during configuration.

| Zaphod contact | MCU / rail | TrackPoint contact | Signal |
| --- | --- | --- | --- |
| J1.4 | P0.04 | 1 | RESET; optional 1 kΩ series resistor |
| J1.5 | P0.05 | 7 | DATA |
| J1.6 | P0.07 | 3 | CLOCK |
| J1.7 | GND | 4 | Ground |
| J1.8 | Regulated 3.3 V | 2 | Power |

Contact numbers are schematic numbers, not a physical viewing direction.
Confirm orientation and continuity using the
[plan's wiring procedure](display-replacement-plan.md#physical-orientation-and-harness-construction).
J1.3 is P0.12, **not ground**. Leave J1.1–3 and J1.9 unwired.

The keyboard keymap and USB/BLE support remain enabled. Pointer movement uses
ZMK's upstream input listener; button reporting is disabled. The display,
LVGL, Bongo animation, LS0XX and SPI stack are disabled. The firmware releases
unused former display pins P0.08/P0.12/P0.21/P0.23 during application startup.
It cannot control bootloader pin states. Blue LED behavior is unchanged;
Morse device identification is separate future work.

CLOCK and DATA use open-drain output only during host commands. Receive mode
uses plain inputs and the UART pinctrl handoff. No internal pull-ups are
added. The working XIAO prototype had no external pull-ups, but the source of
its idle-high bias is unresolved: measure the Zaphod bus before choosing
whether external pull-ups to 3.3 V are needed.

## Build and artifacts

With Docker running, from this worktree:

```sh
./build-display-replacement.sh
./build-display-replacement.sh zaphod-trackpoint-display-replacement --diagnostic
```

The script uses its own west workspace at
`~/.cache/zaphod-zmk-display-replacement`. It refuses the original shared
`~/.cache/zaphod-zmk` path. Set `ZAPHOD_REPLACEMENT_BUILD_ROOT` to choose another
isolated directory. `ZAPHOD_REPLACEMENT_SKIP_UPDATE=1` is for an already
populated workspace at the pinned revisions; it is not an initial setup.

Release output:

```text
<build-root>/artifacts/zaphod-trackpoint-display-replacement/
  zaphod-trackpoint-display-replacement.uf2
  provenance.json
  .config
  zephyr.dts
  zmk.map
  build.log
```

The diagnostic directory and UF2 append `-diagnostic`. The diagnostic image
enables assertions and USB logging through Zaphod's existing CDC ACM console.
Do **not** add the upstream `zmk-usb-logging` snippet to this legacy Zaphod
board: it expects a missing `zephyr_udc0` label and declares another console.
The standalone XIAO target retains that snippet.

CI builds all four `build.yaml` targets, runs host driver tests and the
incompatible-shield checks, and adds the replacement diagnostic image.
Artifacts include generated configuration, link map, dependency revisions,
source and UF2 hashes. Local validation used ZMK
`edf5c0814fd3ea202e43aad2d68fd32e882a518c`, Zephyr
`dacab4875df72109b96cc8977547a0dc04875bcd`, and the pinned driver plus the
[tracked patch](../../patches/README.md), with Zephyr SDK 0.16.9/GCC 12.2.
The container's `stable` tag is mutable; use `ZMK_BUILD_IMAGE` with a fixed
image digest when byte-for-byte toolchain reproducibility is required.

To build a regression target with the same checked patch pipeline:

```sh
./build-display-replacement.sh zaphod
./build-display-replacement.sh zaphod-trackpoint
./build-display-replacement.sh xiao-trackpoint-test
```

The existing `build-local.sh` and `build-trackpoint-test.sh` retain their old
workflow and do not apply this patch. Use the new script to reproduce this
worktree's CI results. Open-drain remains disabled in the two older TrackPoint
targets; shared command/receive reliability fixes apply to both.

## Software validation

Local checks cover:

- Release, assertion-enabled diagnostic, base Zaphod, spare-pad Zaphod, and
  standalone XIAO compilation.
- Generated DTS/Kconfig/link-map checks: pin ownership, UART/GPIOTE settings,
  display removal, keyboard matrix, battery ADC and storage/boot partitions.
- Host execution of the actual patched PS/2 source in both output modes:
  mutex ownership, immediate response, bad ACK, missing clocks/response,
  pin-mode failures, duplicate completion, stale timeout, receive bursts,
  overflow and UART-error handling.
- Rejection of both orders of incompatible shields, patch application and
  idempotence, and refusal to overwrite unexpected driver edits.
- Shell/Python syntax and GitHub Actions validation with actionlint.

The host harness models Zephyr API contracts; it does not simulate interrupt
latency, electrical timing or BLE scheduling. Existing upstream devicetree
unit-address and deprecated Kconfig warnings remain. XIAO also has existing
console/serial assignment warnings. The macOS host compiler warns about an
unchanged upstream non-prototype UART declaration.

## Bring-up and known limitations

Use the diagnostic image for initial USB-powered characterization and the
release image for current measurements. Follow the
[staged hardware checks](display-replacement-plan.md#stepwise-bring-up-and-rollback)
for keyboard-only operation, voltage/pull-ups/reset waveforms, cold starts,
combined typing/movement over USB and BLE, and battery/sleep behavior.
TrackPoint-only wake and automatic recovery after a bus fault are not
implemented.

A receive queue overflow or UART error during runtime latches a pointer fault
until a full keyboard reset. The driver discards subsequent movement rather
than parsing across missing bytes. The upstream framing-error exception for
byte `0xFA` remains. Runtime recovery and stronger packet resynchronization
need measured fault traces before extending this version. A stopped pointer
must be investigated with logs; these software checks are not hardware
qualification.

Before loading a display-enabled rollback image, power off and remove the
TrackPoint from the reused display pins. Restore the display wiring and use
the saved known-working base firmware. Do not erase settings by default.
