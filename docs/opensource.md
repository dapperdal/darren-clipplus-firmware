# Open-source and GitHub release requirements

## What this project is

The desktop simulator and target-independent support code are project code.
The Clip+ target build is a modified Rockbox distribution: Rockbox supplies the
launcher/runtime, target drivers, audio/DSP paths, codecs, storage support, and
the build/packaging conventions; the focus shell replaces the first user-facing
application path. It is not a stock Rockbox-branded UI.

The fork is therefore distributed under GPL-2.0-or-later, while every Rockbox
file keeps its original notice. The project name, Darren branding, and custom
visual assets do not change Rockbox's license obligations.

## Corresponding source

Anyone receiving a compiled fork must be able to obtain the complete matching
source, including the unmodified Rockbox files, the five modified upstream
files, the five focus-shell files, build configuration, and source for any
included codec or utility. A GitHub release should either:

1. include the complete fork source as a tracked directory or release source
   archive; or
2. provide a prominent, durable written offer or direct source URL that stays
   available for the applicable GPL source-offer period and identifies the
   exact source commit and build configuration.

The preferred repository layout is `rockbox/` as a source snapshot or subtree,
with the project files at the repository root. A Git submodule alone is not a
complete corresponding-source offer unless the referenced commit is stable,
public, and the release explicitly includes the submodule source instructions.

## Other notices

Rockbox's `docs/LICENSES`, `docs/COPYING`, `docs/COPYING-MANUAL`, `fonts/COPYING`,
and `wps/COPYING` contain additional notices. Imported libraries, fonts,
manuals, codecs, and generated resources may have terms beyond the core GPL;
retain and ship the relevant notices from the fork tree. Do not state that all
files are covered by one uniform license.

## What must not be published

Do not commit the user's `clppa.bin`, any production firmware image, device
backup, mounted-player media, private update package, bootloader patch, local
ARM toolchain, or unreviewed generated binary. The public repository should
contain source and reproducible build instructions; installable artifacts
should be separate, explicitly identified releases after review.

## Branding and claims

Describe this as a private custom Clip+ firmware fork based on Rockbox. Do not
call it an official Rockbox release, imply Rockbox endorses it, or claim that a
target image is safe for installation merely because it builds.
