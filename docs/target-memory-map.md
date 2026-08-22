# Reference-derived AS3525v2 layout

The following values are explicit in the authorized Rockbox reference and are used only by the layout-test linker script:

| Region | Value | Reference source |
|---|---:|---|
| DRAM origin | `0x30000000` | `firmware/export/as3525.h` `DRAM_ORIG` |
| DRAM size | `0x00800000` | Clip+ configure case `MEMORYSIZE=8`; `as3525.h` |
| IRAM origin | `0x30800000` | `DRAM_ORIG + DRAM_SIZE` |
| IRAM size | `0x00050000` | `as3525.h` `IRAM_SIZE` |
| Translation-table reservation | `0x00004000` | `as3525.h` `TTB_SIZE`, at DRAM top |
| Application load | `0x30000000` | reference Sansa AS3525 bootloader loads `BOOTFILE` at `DRAM_ORIG` |
| Application vectors | first `.init.text` at load address | reference `app.lds` `.vectors` |

The reference bootloader then jumps to the loaded image. These constants are enough for static layout checking, but do not independently verify this project's reset semantics, MMU/cache state, stack setup, DRAM initialization, variant detection, or boot-file contract. The current `entry.S` is intentionally an infinite loop, so the linked artifact is not usable alpha firmware.

