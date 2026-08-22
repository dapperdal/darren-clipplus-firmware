#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Read-only preflight for a supplied production image or candidate artifact."""

import argparse
import hashlib
from pathlib import Path


def digest(path: Path) -> tuple[int, str]:
    sha = hashlib.sha256()
    size = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            size += len(block)
            sha.update(block)
    return size, sha.hexdigest()


parser = argparse.ArgumentParser(description="Read-only Clip+ image preflight")
parser.add_argument("image", type=Path)
parser.add_argument("--expected-sha256")
args = parser.parse_args()

if not args.image.is_file():
    raise SystemExit(f"ERROR: not a regular file: {args.image}")
size, sha256 = digest(args.image)
print(f"path={args.image}")
print(f"size={size}")
print(f"sha256={sha256}")
if args.expected_sha256 and sha256.lower() != args.expected_sha256.lower():
    raise SystemExit("ERROR: SHA-256 mismatch; no write action was performed")
print("preflight=pass (read-only; no install or flash performed)")
