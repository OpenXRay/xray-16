#!/usr/bin/env python3
"""Fingerprint the exact engine gamedata copied into an Android APK."""

import argparse
import hashlib
from pathlib import Path


def fingerprint(root: Path) -> str:
    digest = hashlib.sha256()
    marker = root / "openxray-bundle.sha256"
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path == marker:
            continue
        relative = path.relative_to(root).as_posix().encode("utf-8")
        digest.update(len(relative).to_bytes(4, "big"))
        digest.update(relative)
        content = path.read_bytes()
        digest.update(len(content).to_bytes(8, "big"))
        digest.update(content)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    root = args.root
    if not root.is_dir():
        parser.error(f"missing asset directory: {root}")
    marker = root / "openxray-bundle.sha256"
    marker.write_text(fingerprint(root), encoding="ascii")
    print(f"Android engine gamedata fingerprint: {marker.read_text(encoding='ascii')}")


if __name__ == "__main__":
    main()
