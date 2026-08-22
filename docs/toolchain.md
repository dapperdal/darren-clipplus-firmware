# Target toolchain provenance

The local target compiler is installed through Homebrew, not copied into the project:

- Formula: `arm-none-eabi-gcc`
- Version: 16.2.0
- Binutils: GNU Binutils 2.47.20260726
- Host: macOS arm64
- Formula license: GPL-3.0-or-later WITH GCC-exception-3.1
- Homepage: https://gcc.gnu.org

The compiler and binutils are suitable for non-linked ARM object compilation. They do not establish the Clip+ memory map, boot handoff, vendor loader contract, or safe update path. Run `make target-compile` for the current independent firmware object set. The result is explicitly not a bootable or flashable image.

