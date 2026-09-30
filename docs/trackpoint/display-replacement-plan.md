# Replace the Zaphod display with the verified 3.3 V TrackPoint

Status: implementation plan, researched 2026-09-30 against configuration HEAD
`fdb398a961ecc1049e37d658f8f3324bc43adebc`. No firmware was changed, built,
flashed, or published for this plan. Hardware measurements remain gates below.

Remove the entire Sharp display connection and use **J1.6/P0.07 for CLOCK,
J1.5/P0.05 for DATA, J1.4/P0.04 for RESET, J1.8 for regulated VCC, and J1.7
for GND**. The archived schematic supports these five connections without
cutting a Zaphod trace. Implement a separate optional
`zaphod_trackpoint_display_replacement` shield so the existing display and
spare-pad TrackPoint targets remain available for regression and rollback.
Before powered integration, resolve the pinned driver's push-pull host-write
behavior with the explicit, opt-in compatibility patch described below.

## Evidence and limits

| Finding | Evidence / status |
| --- | --- |
| J1 electrical contact map below | **Confirmed in schematic**, by following the drawn wires at J1 and matching their global nets at U1; not inferred from display signal names or text extraction order. |
| Parent's staged J1 ground correction, pad 3 to pad 7 | **Correct for schematic v0.1.2.** J1.3 is `LCD_EXTMD`/P0.12, not ground. The staged parent change was inspected and left untouched. |
| This salvaged TrackPoint works directly at 3.3 V | **Previously verified on XIAO**, USB and battery-powered BLE, per [xiao-wiring.md](xiao-wiring.md) and [README.md](README.md), dated 2026-09-27. Not newly measured here and not a guarantee for other modules. |
| No trace cuts needed | **Schematic-supported design conclusion** for an unmodified v0.1.2 board with the display completely removed. Actual PCB revision, continuity, and any hand-added straps remain unverified. |
| Physical J1 viewing direction, connector pitch, and mating part | **Unverified.** The source is a circuit schematic, not a v1 assembly drawing or PCB footprint. Establish them on the actual board before making a keyed harness. |
| Driver/configuration compatibility | **Source-reviewed**, with existing cache build outputs as references. The new shield and no-display configuration have not been compiled or tested. |
| Rail headroom, effective pull-ups, reset at low battery, sleep/wake, BLE interference | **Unverified on Zaphod**; explicit bring-up gates, not assumptions. |

The authoritative circuit is [the archived one-page schematic](sources/zaphod-v1-schematic.pdf),
title *The Zaphod*, revision **v0.1.2**, KiCad 6.0.4, generated 2022-04-01.
Its SHA-256 is
`5002120fd59fbfd802cd7d468e7d675992426c99346c2d0d4e258e14bbb3cd18`.
It is byte-identical to
`/Users/yren/github/yhren/zaphod/zaphod-v1_scheme.pdf`. J1 is in the lower
middle and U1 in the upper right; both were inspected visually at enlarged scale.
The hardware repository HEAD is `1166eea1262f9413888b38220f7b4b5e139a0716`;
the schematic is an untracked local reference there, so the PDF hash is the
precise source identity. Its `lite/zaphod_lite.kicad_*` files describe a
different XIAO board and must not establish this board's J1 map or orientation.

## Exact J1 map and chosen wiring

`J1.n` below means the **Zaphod schematic connector contact number**. It is
neither an nRF package pin nor a TrackPoint contact number. `U1.n` is the
Holyiot symbol pin number printed in this schematic; physical module-pad
orientation must be checked against the module drawing before probing it.

| J1 contact | Net traced at J1 | nRF GPIO / schematic U1 pin | New connection or safe disposition |
| ---: | --- | --- | --- |
| 1 | `LCD_EXTIN` | P0.21 / U1.50 | Unwired; reserved GPIO, inactive state below |
| 2 | `LCD_DISP` | P0.23 / U1.51 | Unwired; reserved GPIO, inactive state below |
| 3 | `LCD_EXTMD` | P0.12 / U1.52 | Unwired; reserved GPIO; **never use as GND** |
| 4 | `LCD_CS` | P0.04 / U1.12 | TrackPoint contact 1, RESET, preferably through 1 kOhm |
| 5 | `LCD_DIN` | P0.05 / U1.13 | TrackPoint contact 7, bidirectional DATA / UARTE RX |
| 6 | `LCD_CLK` | P0.07 / U1.54 | TrackPoint contact 3, bidirectional CLOCK / GPIOTE |
| 7 | `GND` | Ground rail | TrackPoint contact 4, GND |
| 8 | `VCC` | XC6206P332MR output, also U1.55 | TrackPoint contact 2, nominal 3.3 V; **not VBUS or VBAT** |
| 9 | Explicit no-connect mark | No GPIO/net assigned | Leave isolated; not a second power or ground contact |

At J1 the ground wire from contact 7 crosses the six signal wires **without
junction dots**. Contact 8 connects to VCC; contact 9 ends in a no-connect
cross. These details are why a label-only reading is insufficient.

The five proposed connections use contiguous J1 contacts 4-8. All six former
display signals are GPIO-capable once the display and its firmware owners are
removed. If layout forces another assignment, J1.1/P0.21, J1.2/P0.23, or
J1.3/P0.12 can instead provide RESET after continuity and ownership checks;
P0.04, P0.05, and P0.07 can also exchange CLOCK/DATA/RESET roles with matching
overlay changes. The table above is the sole default harness map.

P0.04/P0.05/P0.07 have no low-frequency-only recommendation in Nordic's
[aQFN73 pin table](https://docs.nordicsemi.com/bundle/ps_nrf52840/page/pin.html).
That makes them a sensible choice for the PS/2 bus compared with the P1.10 and
P1.11 spare-pad alternative, which Nordic marks for standard-drive signals up
to 10 kHz. This is a pin-selection rationale, not a measured BLE guarantee.
Keep trace output disabled because P0.07 also has a TRACECLK function.

### Physical orientation and harness construction

The schematic draws J1 vertically, **9 at the top and 1 at the bottom**. That
is an electrical drawing convention, not permission to number the physical
keyboard header from a particular screen edge. No evidence here establishes
whether the component-side row reads left-to-right as 1-9 or 9-1, or how a
plug's mating face mirrors its solder face.

1. Disconnect USB and battery and remove the display/cable/breakout from J1.
   Photograph the actual J1 from the side on which the harness will be fitted,
   with the keyboard USB connector visible as an orientation reference.
2. Identify the contact with continuity to a known circuit GND, such as J5.4
   or U2.1: this must be J1.7. Independently identify J1.8 by continuity to
   regulator U2.2/VCC, not USB shell or VBUS. The schematic has an `EARTH`
   net separate from GND, so the USB shell alone is not a ground reference.
3. Follow the row away from J1.8 through J1.7: the next contacts must be
   J1.6 (CLOCK), J1.5 (DATA), then J1.4 (RESET). Confirm them to their MCU
   nets/accessible pads in the table, rather than trusting the counting alone.
   On the other side of J1.8, J1.9 must be isolated. Identify the remaining
   end as J1.3, J1.2, J1.1 and mark the annotated photo.
4. Verify every mating connector/cable conductor end-to-end with a meter.
   A ribbon stripe, connector notch, printed signal name, or schematic symbol
   is not proof of physical contact numbering. If the actual hardware lacks
   the expected single row or continuity, stop and obtain that revision's PCB
   data; do not cut traces to make an assumed map fit.
5. For the TrackPoint use the [back photograph](photos/1000020190-back.jpg)
   and [orientation diagram](figures/13s5561-connector-candidate.svg): back
   copper side facing the viewer, connector on the **right**, `CPCP G3 94V-0`
   text upside down, contacts **1 through 8 top-to-bottom**. The earlier
   diagram's candidate/voltage cautions predate the verified 3.3 V test; retain
   its viewing convention, but use this plan and `xiao-wiring.md` for power.
6. Label both ends by signal and contact number. This is a remapped five-wire
   harness, **not** a straight-through 8- or 9-conductor cable. Leave
   TrackPoint contacts 5, 6, and 8 individually insulated. Add strain relief,
   keep wires short, and route away from the Holyiot antenna and key motion.

### Trace, jumper, and electrical changes

- **Required:** unplug/remove the Sharp assembly completely; add the five
  harness wires above. The schematic shows direct GPIO nets to J1 and no
  series components, jumpers, or rail straps that need cutting or bridging.
  Reusing J1 on the keyboard side avoids fine wires on Holyiot edge pads.
- **Conditional:** if a fitted display adapter or prior modification ties a
  reused GPIO to a rail, remove that adapter/strap before attaching the
  TrackPoint. Document any discrepancy and revise the wiring plan for the
  actual board. A trace cut is not part of the default procedure.
- Use the already verified 3.3 V module, one common supply, and common ground.
  Do not leave it powered from the XIAO or another supply while its signals
  are connected to Zaphod. Direct GPIO wiring must never see 5 V, USB VBUS,
  or raw LiPo voltage. Disconnect all power before moving the harness.
- Prefer 1 kOhm in series with RESET. Provide local 100 nF decoupling across
  TrackPoint contacts 2 and 4; add bulk capacitance only as measurements
  warrant. Do not blindly duplicate the module's suspected reset RC network.
- Preserve optional, initially unpopulated 4.7 kOhm pull-ups from CLOCK and
  DATA to **J1.8/VCC**. The working XIAO had no added pull-ups; the effective
  idle-high source is uncharacterized. With the open-drain configuration
  and driver patch below, require clean released-high levels and edges before
  functional testing. If either line floats or rises too slowly, characterize the bias
  and populate these resistors, then repeat the capture. Do not compensate
  by driving the bus high with push-pull GPIO.
- The schematic's XC6206P332MR supplies the keyboard rail. A nominal regulator
  rating is not an available TrackPoint current budget. Measure keyboard,
  radio, TrackPoint, and LED peak/idle load together, plus rail droop and
  battery-end-of-discharge behavior, before battery acceptance.

## Firmware delta to implement later

Keep [config/west.yml](../../config/west.yml) at ZMK `v0.3` and upstream driver
`f6f3b30677caeccc4970bb48f48240d3504d03f7`. The inspected read-only cache has
ZMK `edf5c0814fd3ea202e43aad2d68fd32e882a518c`, Zephyr
`dacab4875df72109b96cc8977547a0dc04875bcd`, and that exact driver revision;
the ZMK and driver worktrees were clean. Record the additional compatibility
patch hash separately: a patched build must not be described as the exact
unmodified driver. Do not update, patch, or build into
`/Users/yren/.cache/zaphod-zmk` as part of implementing this plan.

### Devicetree shape

Create the new shield by adapting
[zaphod_trackpoint.overlay](../../boards/shields/zaphod_trackpoint/zaphod_trackpoint.overlay),
not by combining both shields. Retain the mouse node, upstream
`zmk,input-listener`, `disable-clicking`, 14,400-baud `uart0`, and GPIOTE IRQ 6
priority 0. Change the signal assignments and add display removal:

```dts
/* Design excerpt, not a complete shield file. */
/ {
    chosen {
        /delete-property/ zephyr,display;
    };
};

&ls0xx { status = "disabled"; };
&spi0 {
    status = "disabled";
    /delete-property/ pinctrl-0;
    /delete-property/ pinctrl-1;
    /delete-property/ pinctrl-names;
    /delete-property/ cs-gpios;
};

/* In mouse_ps2: */
/* rst-gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>; */

/* In uart0_ps2_default/group1, TX then RX as in the existing shield: */
/* psels = <NRF_PSEL(UART_TX, 0, 27)>, <NRF_PSEL(UART_RX, 0, 5)>; */

/* In uart0_ps2_off/group1, retain the unused parking pins: */
/* psels = <NRF_PSEL(UART_TX, 0, 27)>, <NRF_PSEL(UART_RX, 0, 26)>; */

/* In uart0/uart_ps2: */
/* scl-gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>; */
/* sda-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>; */
```

The P0.26 RX parking pin and P0.27 unused TX pin are marked unconnected at
U1 in this schematic (U1.22 and U1.19). Reserve both even though neither is
wired to J1. In the pinned driver `PINCTRL_STATE_SLEEP` is used during
**host-to-device commands** to release DATA from UARTE; it is not merely a
deep-sleep setting. Keep `CONFIG_PM_DEVICE=y` so that state exists. CLOCK
remains GPIO; the device generates the PS/2 clock, and DATA moves between
UARTE receive and GPIO transmit. No UART TX wire goes to the TrackPoint.

### Required electrical-driver gate

The existing shields specify only `GPIO_ACTIVE_HIGH` on the PS/2 lines.
Source inspection shows `ps2_uart_configure_pin_*_output()` calls
`gpio_pin_configure_dt(..., GPIO_OUTPUT_HIGH)`; it does not add open-drain
mode. With those flags Nordic configures S0S1, including an actively driven
high. A successful XIAO test does not establish contention-free behavior.

**Do not fix this by putting `GPIO_OPEN_DRAIN` into `scl-gpios`/`sda-gpios`.**
Zephyr's helper ORs DT flags into every call, including the driver's separate
`GPIO_INPUT` calls. The pinned `gpio.h` asserts that input-only mode cannot
have `GPIO_SINGLE_ENDED`, which is part of `GPIO_OPEN_DRAIN`. Disabling that
assertion is not the fix.

Before connecting hardware, implement and review a small compatibility patch
on an isolated copy of upstream driver `f6f3b306...`:

1. Add an opt-in `PS2_UART_OPEN_DRAIN` boolean under `if PS2_UART` in the
   driver's `src/drivers/ps2/Kconfig.uart`, default `n` to preserve existing
   targets. This is a **proposed new symbol**, not one present at HEAD.
2. In both `ps2_uart_configure_pin_scl_output()` and
   `ps2_uart_configure_pin_sda_output()`, add `GPIO_OPEN_DRAIN` to the output
   flags only when that option is enabled. Retain active-high DT flags and
   leave the input helpers as `GPIO_INPUT` with no single-ended flag.
   Zephyr's Nordic backend maps the output open-drain flag to S0D1: low is
   driven, high is released. Audit all transitions, including input sensing
   for ACK/CLOCK, for direction and error handling.
3. Enable `CONFIG_PS2_UART_OPEN_DRAIN=y` only for the replacement shield.
   Store the patch in this repository, apply it reproducibly after fetching
   the pinned module in local/CI isolated workspaces, and fail the build if
   it does not apply cleanly. Record upstream and patch hashes. Do not edit
   the shared cache or silently change the manifest to an unreviewed fork.
4. Build with assertions enabled as a diagnostic check and inspect GPIO
   drive mode and released-high waveforms through host commands and both
   pinctrl transitions. Pinctrl can rewrite DATA's GPIO configuration.
   Validate optional pull-ups with the revised circuit before adoption.

This patch is **source-supported but neither implemented nor hardware-tested**
here. It is a dependency of the electrically qualified implementation, not a
claim that the unmodified driver already meets open-drain PS/2 behavior. If
using the exact unmodified driver is mandatory, the no-display build can be
evaluated without the module, but powered integration remains blocked at this
gate pending another reviewed electrical solution.

RESET retains the driver's separate push-pull path: high for approximately
600 ms, then low. The driver clears RESET's DT flags, so flags cannot invert
that sequence.

### Disable the whole display path

The new shield's `.conf` should request:

```conf
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_ZAPHOD_BONGO_CAT=n
CONFIG_DISPLAY=n
CONFIG_LVGL=n
CONFIG_LS0XX=n
CONFIG_SPI=n
CONFIG_PM_DEVICE=y
CONFIG_UART_INTERRUPT_DRIVEN=y
# New symbol supplied by the required, tracked compatibility patch:
CONFIG_PS2_UART_OPEN_DRAIN=y
```

`CONFIG_PS2_UART` and `CONFIG_ZMK_INPUT_MOUSE_PS2` are selected/defaulted by
the enabled compatible nodes/module; verify them in the generated `.config`.
The driver selects serial, runtime UART configuration, and `UART_ASYNC_API`,
but actually calls the interrupt-driven UART API. Verify
`CONFIG_UART_0_INTERRUPT_DRIVEN=y`; copying only an async-UART example is
insufficient. Keep the upstream input listener rather than enabling the
driver's legacy `zmk,input-listener-ps2`. Keep buttons and press-to-select off.

Disabling the display node alone does not free its firmware pins, and setting
`CONFIG_ZMK_DISPLAY=n` alone leaves board-default cleanup to resolve:

1. In `boards/arm/zaphod/zaphod_defconfig`, retain the base display default
   `CONFIG_ZMK_DISPLAY=y`, but move the unconditional display-only settings
   `CONFIG_LV_USE_THEME_MONO=y`, `CONFIG_LV_COLOR_CHROMA_KEY_HEX=0x00FF00`, and
   `CONFIG_ZAPHOD_BONGO_CAT=y` to conditional Kconfig defaults under
   `if ZMK_DISPLAY`. Preserve their current values for display builds; the
   chroma key already has that upstream default, so the redundant assignment
   can instead be removed. This prevents disabled-LVGL dependency warnings.
2. In `boards/arm/zaphod/Kconfig.defconfig`, keep all display choices/defaults
   conditional on `ZMK_DISPLAY`; review the currently outer custom-status
   choice and its label selection when making this cleanup. In the board
   `Kconfig`, Bongo's new default belongs inside its existing display guard.
3. In `boards/arm/zaphod/CMakeLists.txt`, guard the two LVGL include-directory
   calls with `if(CONFIG_ZMK_DISPLAY)`; retain the existing conditional source
   inclusion. Require no status-screen, Bongo widget, or Bongo image object
   in the replacement ELF. Do not leave LVGL enabled just to avoid warnings.
4. Remove the chosen display property and disable **both** `ls0xx` and
   `spi0` in the shield. The old pinctrl definitions can remain unreferenced
   in the base board; they must not be applied by any enabled device.

Do not disable `CONFIG_GPIO`, input, ADC/battery sensing, USB, BLE, or the
matrix along with SPI/display. WPM may resolve off once Bongo is disabled;
it is not needed by the TrackPoint.

### Peripheral ownership and remaining pins

| Resource | Required replacement state / check |
| --- | --- |
| P0.04 / P0.05 / P0.07 | RESET / DATA / CLOCK only; no SPI, display CS, trace, QSPI, or second GPIO consumer may configure them. |
| P0.12 / P0.21 / P0.23 (J1.3 / .1 / .2) | No wire attached and no display task toggling them. Prefer `GPIO_DISCONNECTED` (input buffer disconnected, no pulls) for unused pads; apply via shield-specific initialization if bootloader/previous pin owners do not leave that state. Do not tie these GPIOs directly to a rail. |
| P0.08 | Old SPI MISO selection, although J1 has no MISO contact and U1 marks this pin NC. With `spi0` disabled, leave unclaimed/disconnected; do not infer J1.9 is MISO. |
| P0.26 / P0.27 | Reserve for RX parking / unused TX. Do not reuse for LED or another device while this driver runs. |
| UART0 / UARTE0 | Exclusive PS/2 receiver at `0x40002000`, IRQ 2. No physical-UART console, shell, split transport, or second PS/2 instance. |
| SPI0 / I2C0 | Both nodes use `0x40003000`, IRQ 3; keep both disabled here. They share a hardware block with each other, **not with UARTE0**. The conflict with the old display is GPIO ownership. |
| USB console | Keep `zephyr,console = &cdc_acm_uart`; CDC ACM uses USB, not the physical UART0 pins. USB logging can be a diagnostic build variant. |
| GPIOTE / BLE priorities | Keep IRQ 6 priority 0; verify the driver's effective `BT_CTLR_LLL_PRIO=1`, `BT_CTLR_ULL_HIGH_PRIO=2`, `BT_CTLR_ULL_LOW_PRIO=2`. Audit all other enabled interrupt priorities and load-test rather than assuming the one GPIOTE override guarantees timing. |
| Blue LED P1.09 | Remains available on its existing `blue_led` node, connected to D3/R6 in the schematic. Preserve it as the status/feedback integration point. Detailed LED behavior, event ownership, and power policy belong to the separate LED plan. |
| Board RESET P0.18 and SWD | Preserve for recovery; TrackPoint RESET on P0.04 is a different signal. |
| Matrix and battery ADC | Preserve all existing assignments; none of the six freed display GPIOs is a matrix line in the base DTS. |

Nordic documents configurable UARTE GPIO routing and requires one peripheral
owner per driven pin in its [UARTE specification](https://docs.nordicsemi.com/bundle/ps_nrf52840/page/uarte.html).
The addresses above are also confirmed in the pinned Zephyr
`dts/arm/nordic/nrf52840.dtsi`. Do not migrate to UART1 merely because SPI0
was enabled; that would introduce an unnecessary driver/configuration change.

No first-boot, bootloader, or deep-sleep pin waveform has been measured here.
If explicit unused-pad initialization is needed, add shield-scoped CMake/C
with an initialization hook that checks GPIO readiness and configures only
P0.08/P0.12/P0.21/P0.23 as disconnected. Do not use a blanket GPIO reset that
can clobber PS/2, matrix, USB, SWD, or the LED. Confirm its timing against
bootloader handoff and suspend/resume; a DTS comment is not an initialization.

### Implementation file list

These are proposed later changes; this planning change adds only this file.

| File(s) | Proposed change |
| --- | --- |
| `boards/shields/zaphod_trackpoint_display_replacement/Kconfig.shield` | Define the new selectable shield; reject combination with `zaphod_trackpoint` in configuration validation. |
| `boards/shields/zaphod_trackpoint_display_replacement/zaphod_trackpoint_display_replacement.overlay` | Single PS/2/mouse/listener path, new pin map, display/SPI disable and chosen deletion. |
| `boards/shields/zaphod_trackpoint_display_replacement/zaphod_trackpoint_display_replacement.conf` | Pointing on; complete display stack off; PM and interrupt-driven UART requirements. |
| `boards/arm/zaphod/zaphod_defconfig`, `Kconfig.defconfig`, `Kconfig` | Conditional display/Bongo defaults that preserve normal display builds. |
| `boards/arm/zaphod/CMakeLists.txt` | Guard display-specific includes and verify existing conditional sources. |
| `patches/ps2-uart-opt-in-open-drain.patch` | Proposed patch against driver `f6f3b306...`: opt-in Kconfig symbol and open-drain flags in output helpers only. |
| New `scripts/build-display-replacement.sh`, and `.github/workflows/build.yml` | Apply and validate that patch before compiling; the current reusable workflow does not implement this step. Use explicit isolated west setup, pinned fetch, patch check/application, matrix builds, and artifact collection, retaining the same ZMK/Zephyr versions. |
| New shield `CMakeLists.txt` and `unused_display_pins.c`, only if needed | Explicit unused-pad initialization scoped to this shield; no LED behavior. |
| `build.yaml` | Add the replacement artifact alongside every existing entry. |
| `docs/trackpoint/README.md`, this plan, and later wiring/measurement records | Link the two mutually exclusive integration choices and record actual J1 orientation/results. Keep historical XIAO verification distinguishable from new measurements. |

No baseline `.dts` pin remap, keymap rewrite, or manifest revision change is
needed. The tracked patch is an explicit addition to the pinned driver and
requires review and validation. Never select both TrackPoint
shields: the driver uses instance 0 and static state, and two overlays would
compete for UART0, node labels, and pinctrl.

### Build matrix and verifiable gates

Preserve the current CI's ZMK `v0.3` source version and regression matrix while
adding a reproducible patch step and replacement target. Do local builds in a
**separate writable west workspace** and build directories, with the shared
cache only read or mounted `:ro`.
Do not run `build-trackpoint-test.sh` with its default cache destination.

| Board | Shield / snippet | Artifact | Purpose |
| --- | --- | --- | --- |
| `zaphod` | none | `zaphod` | Existing keyboard + display regression and rollback |
| `zaphod` | `zaphod_trackpoint` | `zaphod-trackpoint` | Existing display + spare-pad TrackPoint regression |
| `seeeduino_xiao_ble` | `xiao_trackpoint_test`, `zmk-usb-logging` | `xiao-trackpoint-test` | Known prototype control |
| `zaphod` | `zaphod_trackpoint_display_replacement` | `zaphod-trackpoint-display-replacement` | New no-display firmware |
| `zaphod` | new shield plus `zmk-usb-logging` | Distinct local diagnostic artifact | Initialization/packet logs; not the battery-current reference |

For the new target, after creating the proposed files, the build shape is
`west build -p always -s <isolated-workspace>/zmk/app -d <new-build-dir>
-b zaphod -- -DSHIELD=zaphod_trackpoint_display_replacement
-DZMK_CONFIG=<this-repo>/config -DZMK_EXTRA_MODULES=<this-repo>`.
Use actual absolute paths in place of the angle-bracket arguments, and append
`-S zmk-usb-logging` before `--` for the separate diagnostic build. Apply and
verify the required driver patch before this command. Also build a diagnostic
variant with `CONFIG_ASSERT=y`; do not mask GPIO API violations.

Require all matrix builds to succeed with no new Kconfig/DT warnings before
any hardware deployment. Inspect and archive each build's manifest revisions,
`zephyr.dts`, `.config`, map file, and artifact hash. In particular:

- Replacement DTS: no `zephyr,display`; `ls0xx`, `spi0`, and `i2c0` disabled;
  UART0 enabled at 14,400; RX P0.05, parked RX P0.26, TX P0.27; CLOCK P0.07,
  DATA P0.05 with active-high DT flags and no DT single-ended flag; RESET
  P0.04; exactly one mouse/listener;
  GPIOTE IRQ 6 priority 0. No other enabled node claims these pins.
- Replacement `.config`: `ZMK_POINTING`, `PS2`, `PS2_UART`,
  `ZMK_INPUT_MOUSE_PS2`, `PM_DEVICE`, `UART_INTERRUPT_DRIVEN`, and
  `UART_0_INTERRUPT_DRIVEN`, and the new `PS2_UART_OPEN_DRAIN` enabled;
  `ZMK_DISPLAY`, `DISPLAY`, `LVGL`,
  `ZAPHOD_BONGO_CAT`, `LS0XX`, and `SPI` disabled. Verify BLE priorities and
  CDC console selection; no duplicate/legacy input listener.
- Replacement link map: no status screen/Bongo objects or LVGL/display
  runtime. Confirm keyboard, pointing HID, and USB/BLE endpoints remain.
- Existing Zaphod targets retain display/Bongo configuration, pin maps, and
  their previous output behavior (`PS2_UART_OPEN_DRAIN=n`). This preserves
  regression scope without claiming their existing bus drive is qualified.
  A deliberate dual-shield build must be rejected clearly. `git diff --check`
  must pass, and the original parent staged ground correction must survive.

## Stepwise bring-up and rollback

Each gate produces a measurement record or build artifact. These are future
actions, not claims that this planning task performed them.

1. **Record the baseline and recovery route.** Save the known-working UF2,
   its configuration/hash, board revision, display wiring photo, keymap, and
   BLE endpoint behavior. Verify how the board enters its existing bootloader
   and how SWD is reached. Preserve the verified XIAO harness as a control.
   Exit: recovery is documented before any rewiring.
2. **Identify and isolate.** With USB/battery disconnected, remove the display
   and establish physical numbering as above. Record resistance/continuity
   from J1.7 to GND, J1.8 to U2.2, and J1.4/.5/.6 to the stated MCU nets;
   check all adjacent contacts and VCC-to-GND for shorts. Photograph the
   labeled actual orientation. Exit: no unresolved map or isolation issue.
3. **Implement and build.** Complete the proposed firmware files in an
   isolated workspace and pass the full matrix/static gates. Review the
   specific replacement artifact against the harness table. Exit: a
   reproducible no-display artifact, distinct from both existing Zaphod UF2s.
4. **Check the keyboard alone.** Keep TrackPoint and display disconnected,
   use controlled USB power, and deploy the replacement artifact in a later
   implementation session. Verify every key and USB/BLE keyboard operation;
   measure J1.8 against J1.7 and inspect reused/unused pins during power-up,
   MCU reset, bootloader entry/exit, idle, and sleep/wake. PS/2 device-not-found
   logs are expected with no TrackPoint. Exit: nominal 3.3 V rail, no display
   SPI activity, no conflicting driver or unexplained rail-level strap.
5. **Connect and capture startup.** Power off, add the verified five-wire
   harness, current measurement, and local decoupling. Power through a
   current-limited setup sized from the baseline and module measurements.
   Capture RESET high for about 600 ms then low, released-high CLOCK/DATA,
   initialization responses (`AA 00`/command acknowledgments), and valid
   motion packets. Inspect open-drain host-command operation and both DATA
   pinctrl states. Exit: repeatable initialization on cold power cycles and
   MCU-only reset; no overheating, supply collapse, stuck bus, or contention.
6. **Validate pointer and keyboard together.** Test USB movement/signs and
   absence of button events, then BLE movement while typing all keys and
   holding combinations. Compare a sustained session with the XIAO control;
   log packet/resend/framing failures, dropped keys, and BLE disconnects.
   Record a chosen duration and repeat count, e.g. 30 minutes continuous
   use plus 20 cold starts. Exit: no unexplained regressions or accumulated
   packet errors. Keep LED behavior in the separate integration test scope.
7. **Characterize power and sleep.** Use a nonlogging build to measure idle,
   typing, motion, and combined radio load, then battery cold starts and rail
   droop through the intended battery voltage range. J1.8 is unswitched VCC:
   removing the LCD does not add TrackPoint power gating, and MCU deep sleep
   does not prove that the module powers down. Confirm keyboard wake restores
   PS/2; TrackPoint-only wake is not promised by this plan. Compare measured
   battery impact before enabling normal sleep policy. Exit: documented
   acceptable current, voltage margin, and reliable resume, or a separately
   designed power/wake change before adoption.

Stop on any mapping contradiction, unexpected current, non-3.3 V signal,
contended line, or lost recovery access. Diagnose with the TrackPoint
disconnected; do not flash a display-enabled image while the TrackPoint is
still attached to reused SPI pins.

**Rollback order:** disconnect USB and battery; remove the TrackPoint harness
from all J1 signals and power; restore any documented adapter changes (the
default plan has no cuts); restore the original display wiring; then load the
saved base `zaphod` firmware using the established recovery method. For a
return to the spare-pad plan, restore that plan's distinct harness before
using `zaphod-trackpoint`. Recheck all keys, display, and USB/BLE operation.
No settings erase is inherently required; preserve bonding/configuration
unless a diagnosed HID/pairing issue requires a separate recovery step.

## Source pointers for implementation review

- Hardware authority: local schematic and hash above; see also
  [source provenance](sources/README.md). The hardware repository's
  `tips_from_peterson.md` recommends existing display lines for faster I/O,
  but the actual contact map comes from the schematic.
- Configuration authority: [Zaphod DTS](../../boards/arm/zaphod/zaphod.dts),
  [defconfig](../../boards/arm/zaphod/zaphod_defconfig),
  [board Kconfig defaults](../../boards/arm/zaphod/Kconfig.defconfig),
  [board CMake](../../boards/arm/zaphod/CMakeLists.txt),
  [current PS/2 shield](../../boards/shields/zaphod_trackpoint/zaphod_trackpoint.overlay),
  and [build matrix](../../build.yaml), all at the configuration HEAD above.
- Pinned driver source: `src/drivers/ps2/ps2_uart.c` (GPIO configuration,
  pinctrl mode switching, UART initialization), `src/drivers/ps2/Kconfig.uart`
  (PM and BLE priorities), and `src/drivers/input/input_mouse_ps2.c`
  (power-on reset) in
  [revision f6f3b306](https://github.com/infused-kim/kb_zmk_ps2_mouse_trackpoint_driver/tree/f6f3b30677caeccc4970bb48f48240d3504d03f7).
  Read from the clean local cache; online retrieval of that exact driver file
  was unavailable during this review.
- Pinned Zephyr source: `include/zephyr/drivers/gpio.h`,
  `drivers/gpio/gpio_nrfx.c`, and `dts/arm/nordic/nrf52840.dtsi`, at the Zephyr
  revision above. ZMK's `app/src/display/Kconfig` selects DISPLAY/LVGL when
  `ZMK_DISPLAY` is enabled; the board adds its own defaults.

Review completed for this document: source/hash cross-check, full schematic
and J1 visual inspection, pin/peripheral/driver source audit, and
`git diff --check`. Firmware compilation, physical orientation confirmation,
continuity, waveforms, power characterization, and hardware acceptance remain
explicit implementation gates.
