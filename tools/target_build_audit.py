#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Read-only audit of prerequisites for a real Clip+ target build."""

import shutil
from pathlib import Path

root = Path(__file__).resolve().parents[1]
reference = Path("/Users/darrenfernandez/Documents/Codex/2026-08-11/realtime-voice-chat-2/work/rockbox")
tools = ["arm-none-eabi-gcc", "arm-none-eabi-ld", "arm-none-eabi-objcopy", "arm-none-eabi-as"]
required = [
    reference / "tools/configure",
    reference / "firmware/export/config/sansaclipplus.h",
    reference / "firmware/target/arm/as3525/sansa-clipplus/lcd-clip-plus.c",
    reference / "firmware/target/arm/as3525/sansa-clipplus/button-clip.c",
    reference / "bootloader/sansa_as3525.c",
    root / "target/as3525v2/linker.ld.in",
]

print("target=SanDisk Sansa Clip+")
print("reference_config=sansaclipplus")
print("reference_output=rockbox.sansa")
print("reference_boot_output=bootloader-clipplus.sansa")
print("reference_scramble=scramble -add=cli+")
print("toolchain:")
missing = []
for tool in tools:
    path = shutil.which(tool)
    print(f"  {tool}={'FOUND ' + path if path else 'MISSING'}")
    if not path:
        missing.append(tool)
print("reference_files:")
for path in required:
    print(f"  {'present' if path.is_file() else 'missing'} {path}")
template = (root / "target/as3525v2/linker.ld.in").read_text(encoding="utf-8")
placeholder = "LENGTH = 0x00000000" in template
print(f"linker_template_placeholder={'yes' if placeholder else 'no'}")
toolchain_ready = not missing
reference_ready = all(path.is_file() for path in required)
print(f"toolchain_ready={'yes' if toolchain_ready else 'no'}")
print("memory_linker_verified=no" if placeholder else "memory_linker_verified=yes")
layout_ready = toolchain_ready and reference_ready and not placeholder
print(f"layout_test_ready={'yes' if layout_ready else 'no'}")
print("target_build_ready=no")
print("reason=entry is inert and hardware startup, drivers, boot handoff, and manual validation remain incomplete")
