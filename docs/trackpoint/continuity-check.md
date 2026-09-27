# Passive continuity and resistance check

Purpose: confirm the candidate connector mapping without applying power.

## Safety and setup

1. Disconnect the TrackPoint from every cable, controller, supply, and laptop.
2. Put the PCB on a nonconductive surface.
3. Use resistance/continuity mode only. Do not use diode-test mode initially.
4. Number contacts exactly as shown in
   [figures/13s5561-connector-candidate.svg](figures/13s5561-connector-candidate.svg).
5. Record actual readings rather than only writing "beep" or "no beep".

The `J7` capacitor on the component side has a printed `+` beside its positive
terminal. Use the metal end terminations as probes, taking care not to bridge
nearby parts.

## First-pass measurements

Fill in [continuity-measurements.csv](continuity-measurements.csv).

### Ground and supply evidence

- Contact 4 to a broad copper ground area or a known ground termination.
- Contact 2 to the `J7` positive terminal.
- Contact 1 to the `J7` terminal opposite the printed `+`.
- Contact 1 to contact 4; a reading near 100 kOhm supports the onboard reset
  pull-down shown in the PTPM754 reference design.

Do not use a mounting-hole ring as the only proof of ground; mechanical rings
may be isolated or connected through the keyboard frame differently.

### PS/2 pull-up evidence

- Contact 2 to contact 3 (candidate VCC to CLOCK).
- Contact 2 to contact 7 (candidate VCC to DATA).

A stable reading around 4.7 kOhm would support onboard pull-ups. A much higher
or changing reading may reflect semiconductor paths or a capacitor and is not
enough by itself to assign the signal.

### Cross-checks

- Contact 2 to contact 4 must not be a short.
- Contact 3 to contact 4 and contact 7 to contact 4 must not be hard shorts.
- Contacts 5, 6, and 8 should not be hard-shorted to ground when no external
  buttons are connected.

## Stop conditions

Do not proceed to powered testing if:

- VCC and GND appear shorted;
- the candidate VCC contact does not connect plausibly to the positive side of
  supply/reset capacitors;
- the candidate RESET contact does not match the `J7` network;
- measurements contradict the reference map; or
- contact numbering/orientation is uncertain.

Photograph the meter probes in place for any surprising result. We can revise
the map from those photographs before risking the module.

## After the passive map is confirmed

The next step is a current-limited 5 V power-up of the TrackPoint alone,
without attaching XIAO GPIO. Observe RESET, CLOCK, and DATA through a logic
analyzer or high-impedance meter/scope. Only after confirming signal levels
should CLOCK and DATA be connected through the 5 V/3.3 V open-drain level
shifter.
