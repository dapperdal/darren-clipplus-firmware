# Dolphin standby (11 November 2026)

The Clip+ blue OLED band has an original pixel-art dolphin scene. A distant pod
swims with independent tail beats, a foreground dolphin grows as it passes, and
a dolphin jumps across a moving waterline. Artwork is generated from our own
pixel shapes by `tools/focus_gen.py`; no external animation frames are included.

Home, Now Playing and radio enter standby after eight seconds without an action.
Any key returns to the previous screen without changing playback. Volume, lock,
shutdown and USB use the existing event handler. No drawing occurs while the
OLED is off; the normal timeout and sleep timer remain active. Settings > Standby
saves an on/off choice. Settings > Dolphins and playback Options > Dolphins open
the scene immediately. Clock, alarm, recorder and other editors do not enter it.

## Validation

- ARM player build: `make -j8` completed for `sansaclipplus`.
- Firmware size: 992,716 bytes; RAM usage reported by the build: 1,494,192 bytes.
- SHA-256: `14669fb69d67b0b9b567d35588ae7d108ba2a4eb70eda13cbe16d84224e62f93`.
- All 43 codecs match the existing device codecs byte for byte.
- Simulator: automatic Home standby and return, manual entry/return, continued
  playback when SELECT dismisses standby, volume adjustment, OLED timeout and
  the saved off setting passed. A complete 200-frame scene was captured.
- USB: repeated connections from Home, standby and playback received every
  acknowledgement, disconnected, and returned to the shell. Darren independently
  confirmed MSC browsing and song copying on the physical player. The earlier
  missing-ack diagnosis was incorrect, and USB code was left unchanged.
- The full previous device firmware directory was backed up and verified before
  install. The copied firmware checksum was verified before safely ejecting.

The installed dolphin build still needs a hands-on boot/control check. Recorder,
alarm, clock, sleep power-off, power-off resume and EQ persistence retain their
hardware checks in `HANDOFF.md`.

## Repeat the simulator checks

```sh
python3 tools/test_focus_dolphins.py PATH_TO_SIM_BUILD --capture
python3 tools/capture_focus_tour.py PATH_TO_SIM_BUILD OUTPUT_DIR
```

The test command backs up/restores simulator configuration and session files.
Its `U 1 N` command automatically unplugs after N tenths of a second, so USB can
be tested while the main thread waits inside the stock USB screen. This helper
thread is never compiled into player firmware. Simulator screenshots report
playback status, volume, storage generation and screen state in the test log.
