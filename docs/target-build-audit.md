# Real target build audit

Audit status: not ready for an alpha delivery candidate.

## What the authorized reference establishes

- Configure target: `sansaclipplus` (target id 66), ARM/AS3525, `rockbox.sansa` application output.
- Reference boot output: `bootloader-clipplus.sansa`.
- Reference image transformation: `scramble -add=cli+`.
- Clip+ target files include AS3525 LCD, button, backlight, and power-management code.
- The reference bootloader loads the application from storage and has target-specific LCD, button, storage, and USB/MSC paths.

These are reference build facts, not proof that this independent firmware can use the same handoff or packaging safely.

## Current environment result

The local environment now has Homebrew-managed `arm-none-eabi-gcc` 16.2.0 and GNU Binutils 2.47.20260726; provenance is recorded in [toolchain.md](toolchain.md). All independent firmware sources compile as ARM EABI5 relocatable objects. The project now has a layout-only linker script using reference-derived addresses; see [target-memory-map.md](target-memory-map.md). Its entry point is intentionally inert and it is not an alpha boot script.

Run the read-only audit with:

```text
make target-audit
```

It reports the exact missing prerequisites and returns status text; it does not download tools, alter the reference tree, write a player, or create a candidate image.

Run `make target-compile` to compile the independent firmware sources for ARM926EJ-S/AS3525v2 as freestanding object files. Run `make target-link` for the non-flashing layout test; it emits an ELF/map only and deliberately links an inert entry point. This is a layout test, not target readiness.

Run `make target-validate` to check ELF32/ARM/EABI5 identity, the `0x30000000` entry symbol, unresolved symbols, and section bounds against the reference-derived DRAM/IRAM regions.

## Blockers before alpha delivery

1. Independently review the reference-derived map, startup state, vector placement, and application handoff.
2. Replace the inert entry with verified startup/DRAM/stack/vector glue only after that review.
3. Implement and test target LCD/button/storage/USB glue against the hardware, retaining recovery-first sequencing.
4. Build an unsigned internal test artifact, inspect its map/size/checksums, and validate boot/recovery behavior on non-production test hardware or a proven safe loader path.
5. Only after target boot, LCD, buttons, internal storage/indexing, MP3 playback/resume, watchdog, and rollback gates pass may an alpha delivery checklist be prepared.

No alpha delivery checklist is prepared yet because these gates are not met.
