# Target-port architecture and recovery plan

## Status

The project now has a host-buildable target boundary, but it does not produce a verified bootable or flashable Clip+ image. The AS3525v2 adapter, linker regions, register map, boot handoff, LCD timing, button GPIOs, storage layout, and RAM placement remain hardware-validation work.

The final interface target is the single large text item menu: Up/Down browse with wrapping, Select enters, Home backs out one level, long-press shortcuts remain available, and small Up/Down visual cues explain the physical controls. Destinations are Music, Radio, Recordings, Settings, Podcasts, and Now Playing. Custom Themes are intentionally deferred.

## Layers

- `firmware/src/clipplus_boot.c`: target-independent header, image CRC32, and recovery preflight logic.
- `firmware/src/clipplus_startup.c`: target-independent startup gates; recovery, LCD, input, and read-only storage precede application start.
- `firmware/include/clipplus_target.h`: target facts and board-operation contract.
- `firmware/src/clipplus_board_stub.c`: no-op host adapter; it cannot access hardware.
- `target/as3525v2/`: unverified Clip+ adapter boundary and linker template.
- `tools/image_preflight.py`: read-only SHA-256/size report for the supplied production image or a future candidate.
- `firmware/src/clipplus_ui.c`: target-independent UI, ready to consume a real framebuffer/button adapter later.

## Recovery-first boot sequence

1. Preserve the original production image in a separately verified backup. Run `make preflight ORIGINAL=/path/to/clppa.bin` and record its size and SHA-256.
2. Reset code samples reset reason and the recovery button chord before mounting storage or launching the normal application.
3. On watchdog/panic reset or held Home+Select, enter a minimal recovery screen and do not touch the media index or update area.
4. Validate candidate header magic, format version, bounds, header CRC32, and payload CRC32 before any handoff.
5. Bring up the LCD with a fixed diagnostic pattern and the buttons with a read-only key report. Add timeouts and watchdog servicing at every stage.
6. Mount internal 8 GB storage read-only first. The normal media model must not require microSD; card support remains optional later.
7. Only after repeated diagnostics pass should normal UI/audio/storage tasks be enabled. Any future update writer must be a separate, explicit, user-confirmed tool with power and checksum checks.

## Current safety boundary

No source target code writes flash, installs a bootloader, mounts a player, or changes the supplied production image. The zero-length linker template is deliberately unusable until the memory map and boot handoff are verified. Local success means only that the architecture and preflight logic compile and test on the host.
