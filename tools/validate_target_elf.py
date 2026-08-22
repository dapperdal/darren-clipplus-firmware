#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Static-only validation for the inert, layout-test Clip+ ELF."""

import re
import subprocess
import sys
from pathlib import Path

elf = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/target/clipplus-layout.elf")
if not elf.is_file():
    raise SystemExit(f"missing ELF: {elf}")

def run(*args: str) -> str:
    return subprocess.check_output(args, text=True)

header = run("arm-none-eabi-readelf", "-h", "-W", str(elf))
sections = run("arm-none-eabi-readelf", "-S", "-W", str(elf))
symbols = run("arm-none-eabi-nm", "-n", str(elf))
undefined = run("arm-none-eabi-nm", "-u", str(elf))

checks = {
    "elf32": re.search(r"Class:\s+ELF32", header) is not None,
    "arm": re.search(r"Machine:\s+ARM", header) is not None,
    "eabi5": "Version5 EABI" in header,
    "entry": "Entry point address:               0x30000000" in header,
    "entry_symbol": re.search(r"^30000000 T clipplus_target_entry$", symbols, re.MULTILINE) is not None,
    "no_undefined": not undefined.strip(),
}

section_re = re.compile(r"^\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+\S+\s+([0-9a-fA-F]+)")
regions = {
    ".vectors": (0x30000000, 0x307FC000),
    ".text": (0x30000000, 0x307FC000),
    ".rodata": (0x30000000, 0x307FC000),
    ".data": (0x30000000, 0x307FC000),
    ".bss": (0x30000000, 0x307FC000),
    ".stack": (0x30800000, 0x30850000),
}
for line in sections.splitlines():
    match = section_re.match(line)
    if not match or match.group(1) not in regions:
        continue
    name, address_hex, size_hex = match.groups()
    address, size = int(address_hex, 16), int(size_hex, 16)
    low, high = regions[name]
    checks[f"{name}_bounds"] = low <= address and address + size <= high

for name, passed in checks.items():
    print(f"{name}={'PASS' if passed else 'FAIL'}")
if not all(checks.values()):
    raise SystemExit(1)
print("static_validation=PASS (layout only; not bootable or flashable)")
