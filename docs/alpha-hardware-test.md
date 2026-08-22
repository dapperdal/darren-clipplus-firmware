# Clip+ alpha hardware test

This is a staged, non-destructive observation checklist. Start with the player in its known-good production state. Do not install or flash this project from these instructions. The current project is not a verified bootable image.

## Before touching the player

- [ ] Original firmware backup exists separately and is unchanged.
- [ ] Backup size and SHA-256 are recorded.
- [ ] Player battery is comfortably charged; use a known-good USB cable.
- [ ] Test microSD is not required for this alpha; use built-in storage first.
- [ ] Record player model/markings and the date.

Optional read-only host check:

```text
make preflight ORIGINAL=/path/to/clppa.bin
```

Expected result: the recorded size and SHA-256 match. This command reads only; it does not install or modify anything.

## Stage A — known-good production baseline

1. Power on normally.
2. Confirm the player reaches its normal home/playback screen.
3. Confirm the display has no persistent lines, missing regions, or flicker.
4. Connect USB and confirm the computer sees the player as USB/MSC storage.
5. Without editing files, confirm internal storage is visible and contains the expected existing media.
6. Disconnect USB using the operating system’s safe-eject action, then power-cycle normally.

Record Pass/Fail and observations before continuing.

## Stage B — read-only hardware observations

With no install or firmware change:

- [ ] Display: text and backlight are stable.
- [ ] Up: one press moves/changes the current selection once.
- [ ] Down: one press moves/changes the current selection once.
- [ ] Left: one press backs/seeks according to the current production screen.
- [ ] Right: one press advances/seeks according to the current production screen.
- [ ] Home: returns/back behavior is consistent.
- [ ] Select: enters/activates the highlighted item.
- [ ] Power: normal power behavior works.
- [ ] Volume Up: volume increases.
- [ ] Volume Down: volume decreases.
- [ ] Hold behavior: only test existing documented production shortcuts; do not invent a firmware chord.

## Stage C — future alpha image, only after verification

This stage is intentionally blocked today. It may begin only when the coordinator has a verified bootable candidate, a proven recovery path, a known memory map, and a written rollback procedure. The candidate must pass size, SHA-256, header, payload, power, and recovery preflight before any install decision.

The install action itself must be supplied separately by the coordinator after those gates pass. Do not use a raw disk-write command, guessed bootloader file, or undocumented vendor update method.

If approved later, test in this order and stop on the first failure:

1. Recovery preflight and recovery-screen visibility.
2. Boot to the single-item UI; confirm large text and Up/Down cues.
3. Test Up/Down wrapping, Select enter, Home/back, and long-press shortcuts.
4. Test USB/MSC visibility without writing files.
5. Scan a small built-in-storage media set incrementally; confirm controls remain responsive.
6. Play an MP3; pause, power-cycle through the documented safe path, and confirm instant resume.
7. Trigger only the documented non-destructive fault/recovery test; confirm safe mode and readable error state.
8. Stop and report results before any broader library or endurance test.

## Result template

Copy this section into the coordinator task after each alpha session.

```text
Clip+ alpha test
Date/time:
Player markings:
Firmware state: production / approved alpha candidate / other
Candidate SHA-256 (if applicable):
Battery/USB setup:

Stage A — production baseline: PASS / FAIL / NOT RUN
  Boot/home:
  Display:
  USB/MSC visibility:
  Internal storage visible:

Stage B — controls: PASS / FAIL / NOT RUN
  Up:
  Down:
  Left:
  Right:
  Home/back:
  Select/enter:
  Power:
  Volume up/down:
  Hold shortcuts:

Stage C — alpha candidate: PASS / FAIL / NOT RUN
  Recovery preflight/screen:
  Boot/UI:
  Up/Down wrapping and cues:
  Select/Home:
  USB/MSC:
  Incremental internal-storage scan:
  MP3 playback:
  Resume after safe power cycle:
  Fault/recovery:
  Rollback readiness:

Observed issues (include exact screen text, button sequence, and timing):
Photos/video available: yes / no
Stopped safely after failure: yes / no
Recommended next action:
```

## Rollback report

If any approved alpha test fails, stop testing, do not retry an install, preserve the original image, record the exact failure, and use only the independently verified recovery/rollback path. Report whether the player still boots, whether USB/MSC remains visible, and whether the original production firmware is restored.

