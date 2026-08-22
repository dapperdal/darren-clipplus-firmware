# AS3525v2 / Clip+ target adapter

This directory is an intentionally incomplete hardware boundary. The local build uses a no-op board adapter so UI and boot validation can be tested without a player. It is not a bootloader, firmware image, or flashable artifact.

The eventual adapter must implement, in this order:

1. reset reason and recovery-button sampling;
2. clock and watchdog setup;
3. GPIO/button readout;
4. LCD reset, controller init, and a read-only test pattern;
5. framebuffer present;
6. storage mount in read-only diagnostic mode;
7. normal application handoff.

The board contract also reserves explicit adapters for read-only internal-storage mounting and MP3 PCM start/stop. The current host adapter reports read-only storage available but never starts audio; real implementations must be added and manually validated before any alpha claim.

Every stage needs a visible recovery path and a timeout. Register addresses, linker placement, RAM/IRAM use, LCD timing, and boot handoff remain unverified and must be derived from board measurements/reference code before target compilation is enabled.
