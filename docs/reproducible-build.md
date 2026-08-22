# Reproducible build and release checklist

## Source inputs

- Pin the Rockbox source to the exact commit recorded in `NOTICE.md`.
- Record the target configuration, host OS, compiler family/version, binutils
  version, and relevant configure options.
- Build the ARM toolchain outside the repository; never commit it.
- Build codecs from corresponding source and verify their expected `.rockbox`
  paths. The package verifier must fail if the MP3 codec is absent.
- Keep the original production firmware and bootloader-patch inputs outside the
  repository. They are recovery inputs, not project source.

## Verification before release

1. Run `make test` for the host simulator.
2. Run the target build and record the linker, section, architecture, and image
   bound checks.
3. Run `tools/verify_focus_package.py` against a staging directory.
4. Record SHA-256 values for the application image and every shipped codec.
5. Inspect the source diff against the pinned Rockbox commit and confirm that
   all modified files retain their upstream license headers.
6. Produce a source archive containing the complete matching fork.

Build success is not hardware validation. The release notes must separately
state whether boot, LCD, buttons, storage, radio, recording, codecs, alarm
wake, and rollback have been manually verified on a real Clip+.

## Installation and recovery warning

Application-only updates assume the already-installed compatible bootloader and
must never be confused with a bootloader patch. Never publish a flash command
that overwrites the production firmware without an exact-device backup,
checksum verification, tested recovery route, and a written rollback procedure.
Do not include `clppa.bin` or a bootloader artifact in the normal source tree.
