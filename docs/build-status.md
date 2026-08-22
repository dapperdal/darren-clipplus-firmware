# Build status

## Milestone 0 — baseline and UI slice

Status: complete for host simulation.

The repository now contains a standalone C UI module, a host simulator, a 128×64 framebuffer renderer, a simple icon set, playback-status footer, and long-press navigation behavior. The simulator is the first executable acceptance target while the hardware port remains unverified.

## Next milestones

1. Add an isolated target abstraction for clock, GPIO, LCD, buttons, and persistent storage.
2. Validate boot handoff and memory layout against a sacrificial/testable recovery path; never overwrite the production image during development.
3. Bring up LCD and buttons with a read-only diagnostic screen.
4. Add storage discovery and a resumable media index with crash-safe checkpoints.
5. Add audio playback, then radio and recording behind independent feature gates.
6. Package signed/versioned update and recovery artifacts only after hardware validation.

FLAC is now tracked as a supported planned playback format. The decoder, target memory accounting, and verification corpus remain future work; see [audio-formats.md](audio-formats.md).

Storage scope is now internal 8 GB first. microSD is optional later and is not a dependency for the library, podcasts, long mixes, or resume behavior; see [storage-media.md](storage-media.md).

The host runtime now includes bounded event scheduling, incremental indexing, CRC/generation resume slots, and watchdog fault escalation. These are locally tested but not yet target-integrated; see [runtime-stability.md](runtime-stability.md).

The host startup state machine now enforces recovery, LCD, input, and read-only-storage gates before application start. The user's production baseline passed normal boot, display, USB/MSC, controls, reconnect, and ordinary playback; see [known-good-baseline.md](known-good-baseline.md). A target boot remains unverified.

The target-build audit confirms the authorized Clip+ reference inputs are present, but the local ARM cross-toolchain is missing and the independent linker template is still intentionally blocked on a verified memory map. No alpha delivery checklist is prepared; see [target-build-audit.md](target-build-audit.md).

The ARM toolchain is now installed and verified, and all independent firmware sources compile to ARM EABI5 relocatable objects. Linking remains intentionally blocked by the unverified memory map, startup/vector placement, and boot handoff.
