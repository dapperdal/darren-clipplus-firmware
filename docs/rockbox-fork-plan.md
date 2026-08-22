# Rockbox Clip+ focus fork

The fastest credible route is now an isolated Rockbox worktree with the
Clip+ low-level target support intact and a project-original first application
screen. The upstream reference checkout is untouched; the fork is built in
`work/rockbox-focus-fork` with output in `work/rockbox-focus-build`.

## Reference build result

The official Clip+ application and separate bootloader both build with the
local `arm-none-eabi-` toolchain. The reference artifacts are retained only
as low-level/build validation:

- `work/rockbox-build-clipplus/rockbox.sansa`
- `work/rockbox-build-clipplus-bootloader/bootloader-clipplus.sansa`

The application ELF is ARM EABI5 and is approximately 949 KiB text, 9.6 KiB
data, and 507 KiB BSS. The bootloader ELF is approximately 67 KiB text,
4.9 KiB data, and 213 KiB BSS. GCC 16.2.0 is newer than the reference's
recommended 9.5.0, so compiler-version compatibility remains an alpha gate.
Known compiler warnings were emitted, but no dependency or link failure
stopped either build.

The stock/reference image must not be installed. No image has been copied to
or flashed on the physical player.

## Custom first-boot integration

The isolated fork adds `apps/focus_menu.c` and selects it at the application
handoff in `apps/main.c` for `SANSA_CLIPPLUS`. This is the smallest safe seam:
the existing AS3525v2 startup, LCD, buttons, storage, USB, power, and audio
drivers remain unchanged while the first user-facing screen is replaced.

The first shell is deliberately narrow and quiet:

- one large text item at a time;
- Up/Down wrapping with visible `UP`/`DOWN` cues;
- Select enter and Home/back behavior at the top level;
- six destinations: Music, Radio, Recordings, Settings, Podcasts, Now Playing;
- no plugin browser, full root menu, themes, network, or notification surface.

The current fork milestone proves the boot-facing UI code compiles in the
full Rockbox application build. Destination handoffs are intentionally the
next integration slice: they must call existing playback, radio, recording,
settings, WPS, and storage/library services without reintroducing a menu maze.
The stock root menu remains in source as a recovery/debug fallback, but is not
the Clip+ default path in this fork.

Settings now has a direct compact EQ surface in `apps/focus_eq.c`: five visible
bands, immediate bounded Up/Down gain adjustment, Select band advance, Home
back, and Rockbox DSP coefficient updates. Its target visual and audible
behavior remain unverified until manual hardware testing.

## GPL and alpha gates

Rockbox is GPL software. Any distributable fork must retain upstream notices,
identify the exact source revision, publish corresponding source and build
instructions as required by the applicable GPL version, and distinguish
project-original files. The simulator/core remains independent and does not
copy Rockbox source.

Before installation: build both application and bootloader, audit architecture
and sections/symbols, record deterministic checksums, preserve and verify the
original production image, review the recovery/rollback procedure, and pass
staged manual tests for boot, LCD, every button, internal-storage scan, MP3
playback/resume, USB/MSC, fault recovery, and rollback. No current artifact
has passed the hardware-boot gate.
