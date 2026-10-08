#!/usr/bin/env python3

import argparse
import hashlib
import json
import re
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Create the Android release update manifest.")
    parser.add_argument("--apk", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--version-file", type=Path, default=Path(__file__).with_name("PORT_VERSION"))
    args = parser.parse_args()

    version_lines = args.version_file.read_text(encoding="utf-8").splitlines()
    if len(version_lines) != 2 or not re.fullmatch(r"\d+\.\d+\.\d+", version_lines[0]):
        parser.error("Android version file must contain a semantic version and a version code")
    try:
        version_code = int(version_lines[1])
    except ValueError:
        parser.error("Android version code must be an integer")
    if version_code < 1 or not args.apk.is_file():
        parser.error("Android APK must exist and version code must be positive")

    digest = hashlib.sha256()
    with args.apk.open("rb") as apk:
        for block in iter(lambda: apk.read(1024 * 1024), b""):
            digest.update(block)

    manifest = {
        "schema_version": 1,
        "application_id": "org.openxray.stalker",
        "version_name": version_lines[0],
        "version_code": version_code,
        "min_sdk": 26,
        "apk_file": args.apk.name,
        "apk_size_bytes": args.apk.stat().st_size,
        "apk_sha256": digest.hexdigest(),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
