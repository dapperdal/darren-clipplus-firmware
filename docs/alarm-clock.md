# Optional alarm-clock service

## Design intent

Alarm is an optional, low-overhead background service for the quiet focus-player workflow. It is disabled by default, has no network or notification dependency, and does not create a plugin/menu maze. A scheduled alarm may notify while music continues; the service does not stop, pause, or change playback.

The target-independent implementation validates a weekday mask and 24-hour time, fires at most once per day, and returns a small action (`NOTIFY`, `WAKE_REQUEST`, or an explicit blocked result). It is tested in the host simulator.

## Current Clip+ shell status

The focus shell now exposes `ALARM` as a first-class destination. Select opens
the existing Rockbox RTC alarm editor through a controlled wrapper and Home
returns to the focus shell. This is the safest currently buildable integration
point, but it is not yet the finished reminder experience: reminder audio while
music is playing, the exact notification behavior, and wake-from-off still
require staged Clip+ testing. The option remains optional and off by default.

## Sleep/off behavior

While the firmware is active, an alarm can notify once the time source is valid. In sleep or off states, the service returns `BLOCKED_UNVERIFIED_WAKE` unless both hardware RTC retention and wake capability have been verified. With those capabilities verified, it returns `WAKE_REQUEST`; this is only an intent for the target power manager, not a claim that the Clip+ currently wakes from off.

The current project has not verified the Clip+ RTC alarm path, low-power retention, wake GPIO/interrupt behavior, buzzer/audio startup latency, or power consumption. Therefore wake-from-off is not implemented or promised.

## Target integration gates

- Verify RTC source, timezone/clock-setting behavior, retention through sleep, and drift.
- Verify wake from sleep and off on real hardware with a visible recovery path and watchdog timeout.
- Decide the quiet notification output: short audio cue, existing playback ducking, or display-only cue; preserve playback continuity.
- Persist the optional config with the same CRC/generation state discipline as resume data.
- Test disabled/default behavior, invalid times, weekday boundaries, late polling, duplicate ticks, power transitions, and playback continuity.
