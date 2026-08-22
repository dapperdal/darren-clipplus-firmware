# Speed and stability core

## Implemented host-testable pieces

- Fixed-capacity 16-event queue with FIFO ordering and an explicit dropped-event counter. No heap allocation or unbounded work occurs in the queue.
- Incremental media scan with a caller-supplied per-step budget and a 64-item bounded index for the first core. The internal 8 GB volume is the primary root; microSD is not required.
- Strict path/extension validation for MP3 and FLAC. FLAC is indexed as `FLAC_PLANNED` and is not treated as decoded playback.
- Dual-slot resume state with generation numbers, CRC32 validation, and fallback to the last intact slot after simulated corruption. Resume positions are per media ID and bounded by duration.
- Watchdog policy with explicit kick/expiry checks and a configurable consecutive-fault threshold for recovery escalation.

## Product behavior defaults

The alpha is intended as a quiet, no-disturbance MP3 player: no network or notification assumptions, no background feature that can interrupt playback, instant resume, predictable physical controls, and short bounded startup work. Library scanning should yield between small batches; playback and input events take priority over indexing.

## Alpha-readiness gates

An alpha may be considered only after all of these are true:

1. Verified boot handoff, memory map, LCD/button read-only diagnostics, and recovery chord on real hardware.
2. Original firmware backup and candidate artifacts have recorded SHA-256 values; update tooling is explicit, checksummed, power-aware, and reversible.
3. Internal-storage mount and bounded incremental indexing survive power loss, malformed paths, duplicates, full index conditions, and storage errors without blocking controls.
4. Resume state survives interrupted writes, corrupted newest slot, reset during playback, and long podcast/mix positions.
5. Watchdog/recovery returns to a visible safe mode after repeated faults, with no destructive automatic repair.
6. MP3 playback is sustained without underruns while indexing; FLAC remains disabled until its separate decoder/resource gates pass.
7. The single-item UI passes Up/Down wrapping, Home back, Select enter, long-press shortcuts, and readable cue checks on the 128×64 LCD.

## Rollback checklist

- Stop: do not flash or install if any preflight, checksum, power, or recovery check fails.
- Preserve: keep the original production image and its SHA-256 offline and unchanged.
- Recover: use the verified recovery path and the explicit candidate version/checksum record; never overwrite the only known-good copy.
- Revert: return to the known-good firmware using the vendor-supported recovery method only after the recovery path is independently proven.
- Record: capture reset reason, visible error code, candidate checksum, and storage state for diagnosis.

