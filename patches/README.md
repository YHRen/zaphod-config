# Pinned PS/2 transport patch

`ps2-uart-reliability.patch` applies to
`infused-kim/kb_zmk_ps2_mouse_trackpoint_driver` at
`f6f3b30677caeccc4970bb48f48240d3504d03f7`. The JSON manifest records the
revision, patch SHA-256 and before/after hashes of both changed files.

`python3 scripts/prepare-ps2-driver.py <isolated-module-checkout>` verifies the
revision and patch hash, applies the patch only to matching original files,
and verifies the result. Repeating it is safe. Unexpected edits to either
patched file cause a failure, not a reset or overwrite. The local wrapper
and CI perform this step before compilation. Keep shared caches untouched.

The patch:

- Adds default-off `CONFIG_PS2_UART_OPEN_DRAIN`, used only in GPIO output
  helpers. Applying GPIO single-ended flags through devicetree would also
  affect input configuration and violate Zephyr's GPIO API contract.
- Keeps the write mutex owned by the calling thread through all retries;
  ISR/workqueue completion uses a semaphore. Arms response capture before
  restoring UART RX, rejects invalid bit ACKs, and completes callers even
  after mode-switch failures.
- Drains an earlier timeout before a new attempt and checks the latest
  absolute clock deadline before aborting an active attempt.
- Replaces a shared callback byte with a 100-byte FIFO so coalesced work
  submissions do not overwrite movement bytes.
- Latches a runtime receive fault on FIFO overflow or UART error, preserving
  the upstream `0xFA` framing exception. Pointer operation then requires a
  full keyboard reset; no silent continuation across a lost byte occurs.
- Preserves negative queue errors and initializes the resend work item.

Run the host contract harness after applying the patch:

```sh
python3 scripts/test-ps2-driver.py <isolated-module-checkout>
```

It compiles the actual driver with mocked Zephyr APIs, for both output modes.
It is not a substitute for assertion-enabled hardware bring-up, interrupt
latency measurements, bus waveforms or sustained BLE testing. See the
[firmware guide](../docs/trackpoint/replacement-firmware.md).
