# Rockbox provenance audit

Audited local fork: `work/rockbox-focus-fork`.

## Findings

- It is a shallow checkout of `https://github.com/Rockbox/rockbox.git` at
  `fd8d6f10a1581492619a765a5cb9eead20314e3b`.
- It contains the complete Rockbox source tree, not only a small library.
- Existing upstream files carry Rockbox GPL notices and must remain intact.
- Five existing Rockbox files are modified: `apps/SOURCES`,
  `apps/keymaps/keymap-clip.c`, `apps/main.c`, `apps/root_menu.c`, and
  `firmware/powermgmt.c`.
- Five focus-shell files are added: `apps/focus_eq.c`, `apps/focus_font.c`,
  `apps/focus_font.h`, `apps/focus_media.c`, and `apps/focus_menu.c`.
- The fork generates `rockbox.elf`, `rockbox.bin`, `rockbox.sansa`, codecs,
  plugins, language archives, and `rockbox-info.txt`; these are not source and
  are excluded from the public repository.
- Rockbox includes additional imported-component notices, especially under
  `docs/LICENSES`, `fonts/COPYING`, and `wps/COPYING`. A release must retain
  them with the complete source snapshot.

## Release decision

The current local project is ready for licensing review and a controlled push,
but it is not yet a complete public source archive because the fork checkout is
kept under `work/` and is ignored. Before publishing a binary release, either
move/copy the exact fork into a tracked `rockbox/` directory or create a stable
source archive/release asset containing it. Do not rely on the user's local
worktree or an unpinned upstream URL as the corresponding-source offer.
