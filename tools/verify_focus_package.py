#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fail-closed verification for the staged Clip+ application update."""
from pathlib import Path
import hashlib
import sys


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_focus_package.py PACKAGE_DIR", file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    files = {
        "firmware": root / ".rockbox" / "rockbox.sansa",
        "mp3_codec": root / ".rockbox" / "codecs" / "mpa.codec",
        "flac_codec": root / ".rockbox" / "codecs" / "flac.codec",
        "info": root / ".rockbox" / "rockbox-info.txt",
    }
    missing = [name for name, path in files.items() if not path.is_file()]
    if missing:
        print("package verification failed: missing " + ", ".join(missing), file=sys.stderr)
        return 1
    for name, path in files.items():
        print(f"{name}: {path.relative_to(root)} sha256={sha256(path)} size={path.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
