# Salvaged TrackPoint module identification

Status: visual inspection completed 2026-09-26; electrical pinout remains
unverified.

## Submitted photographs

- `photos/1000020189-front.jpg` - component/front side.
- `photos/1000020190-back.jpg` - trace/back side and edge contacts.

The original files are retained without modification. Their EXIF timestamps
are 2026-09-26 18:07:51 and 18:08:11 respectively.

## Findings supported by the photographs

- The front silkscreen reads `13S5561 REV.B` (high confidence; confirm with a
  glare-free macro photograph if the exact identifier becomes important).
- The back carries the PCB fabrication marking `CPCP G3 94V-0`.
- The host connector has **eight contacts**.
- The assembly has two main ICs, a crystal/resonator, an electrolytic/tantalum
  capacitor marked `J7`, a dense analogue front end, and labeled test points.
- The layout is a very close match to the documented ThinkPad T400 `LVMV`
  module (`139038L REV.A`) whose main controller is identified as a
  PTPM754DR. The connector location, test-point field, controller, auxiliary
  IC, crystal, and `J7` capacitor are arranged in the same family pattern.

The layout match strongly supports a classic 5 V PS/2 TrackPoint IV design,
but it does **not** prove that the connector pin order or electrical population
is identical. The photographed IC markings are unreadable because of glare
and focus, and the board identifier/revision is different.

## Candidate connector map - do not power from this table

The diagram in [figures/13s5561-connector-candidate.svg](figures/13s5561-connector-candidate.svg)
uses the same viewing orientation as the submitted back photograph: back side
facing the viewer, connector at the right, and the `CPCP G3 94V-0` text upside
down. Contacts are numbered top-to-bottom in that orientation.

| Contact | Candidate signal | Basis |
| ---: | --- | --- |
| 1 | RESET | Position corresponding to T400/LVMV pin 1 |
| 2 | VCC, probably 5 V | Position corresponding to T400/LVMV pin 2 |
| 3 | PS/2 CLOCK | Position corresponding to T400/LVMV pin 3 |
| 4 | GND | Position corresponding to T400/LVMV pin 4 |
| 5 | MB0 / left button | Position corresponding to T400/LVMV pin 5 |
| 6 | MB1 / right button | Position corresponding to T400/LVMV pin 6 |
| 7 | PS/2 DATA | Position corresponding to T400/LVMV pin 7 |
| 8 | MB2 / middle button | Position corresponding to T400/LVMV pin 8 |

This is deliberately labeled a **candidate** map. Complete the passive tests
in [continuity-check.md](continuity-check.md) before connecting a supply.

## Likely onboard reset circuit

The `J7` polarized capacitor is in the same physical location as the
2.2 uF reset capacitor on the documented PTPM754DR-family module. If the
candidate mapping is correct, passive measurements should show:

- contact 2 (VCC) connected to the `J7` positive terminal;
- contact 1 (RESET) connected to the opposite `J7` terminal; and
- approximately 100 kOhm from RESET to GND.

If confirmed, the module already contains the RC power-on-reset network and
the prototype should **not add a second 2.2 uF/100 kOhm network**. The RESET
contact can remain unconnected for ordinary power-on operation, while still
being available for diagnostics or an explicit reset supervisor later.

## Evidence quality and next evidence needed

| Item | Confidence | Next check |
| --- | --- | --- |
| Eight-contact connector | Confirmed | None |
| Board ID `13S5561 REV.B` | High | Glare-free macro photo |
| TrackPoint IV / PTPM754DR family | High, layout-based | Read main IC marking |
| 5 V supply | Probable | Passive mapping, then current-limited power |
| PS/2 protocol | Probable | Logic capture and `AA 00` startup response |
| Connector signals | Candidate only | Continuity/resistance worksheet |
| Onboard RC reset | Probable | J7 continuity and RESET-to-GND resistance |
| Onboard CLOCK/DATA pull-ups | Unknown | VCC-to-signal resistance measurements |

## Reference comparison

The closest documented comparison is the T400/LVMV TrackPoint entry on the
Deskthority TrackPoint Hardware page. Its published back-side map is:

`8 MB2, 7 DATA, 6 MB1, 5 MB0, 4 GND, 3 CLOCK, 2 VCC, 1 RESET`.

Source: <https://deskthority.net/wiki/TrackPoint_Hardware>.

Do not substitute the R61 flying-wire picture or a newer two-piece TrackPoint
pinout; those are mechanically and electrically different assemblies.
