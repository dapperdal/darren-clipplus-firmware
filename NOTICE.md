# Copyright and third-party notice

## Project code

Project-original simulator, target-independent core, focus-shell additions,
build helpers, and documentation are Copyright (C) 2026 Darren Fernandez and
are released under GPL-2.0-or-later. New source files carry SPDX identifiers;
existing upstream-modified files retain their original notices.

## Rockbox

The target fork is based on the Rockbox source repository:

- Project: Rockbox open source jukebox firmware
- Upstream: https://github.com/Rockbox/rockbox
- License: GNU General Public License, version 2 or later, subject to the
  individual file notices and the additional notices under `docs/` in the
  Rockbox tree
- Local source snapshot used for the fork: commit
  `fd8d6f10a1581492619a765a5cb9eead20314e3b`

The fork modifies these existing Rockbox files:

- `apps/SOURCES`
- `apps/keymaps/keymap-clip.c`
- `apps/main.c`
- `apps/root_menu.c`
- `firmware/powermgmt.c`

It adds these project-original fork files:

- `apps/focus_eq.c`
- `apps/focus_font.c`
- `apps/focus_font.h`
- `apps/focus_media.c`
- `apps/focus_menu.c`

The fork still contains the complete Rockbox source tree, including files with
their own copyright, license, font, manual, codec, and imported-component
notices. Do not remove those notices when redistributing the fork.

## Generated and private material

`rockbox.sansa`, codec binaries, bootloader images, the supplied production
firmware, device backups, mounted-player contents, and local toolchains are
build or private-device material. They are deliberately excluded by
`.gitignore` and must not be committed or published as source.
