#!/usr/bin/env python3
"""Fail APK staging when a declared Vulkan shader binary is missing or invalid."""

import argparse
import json
from pathlib import Path
from zipfile import BadZipFile, ZipFile

if __package__:
    from .compile_vulkan_shader import _valid_spirv
else:
    from compile_vulkan_shader import _valid_spirv


def check_assets(shader_root, manifest):
    variants = json.loads(manifest.read_text(encoding="utf-8"))["variants"]
    problems = []
    for variant in variants:
        path = shader_root / variant["output"]
        if not path.is_file():
            problems.append(f"missing {path}")
        elif not _valid_spirv(path.read_bytes(), variant["stage"], variant.get("entry", "main")):
            problems.append(f"invalid SPIR-V stage/entry: {path}")
    return problems


def check_apk(apk, manifest):
    variants = json.loads(manifest.read_text(encoding="utf-8"))["variants"]
    problems = []
    try:
        with ZipFile(apk) as archive:
            if "lib/armeabi-v7a/libmain.so" not in archive.namelist():
                problems.append("missing Android gameplay libmain.so")
            for variant in variants:
                name = "assets/gamedata/shaders/" + variant["output"]
                try:
                    data = archive.read(name)
                except KeyError:
                    problems.append(f"missing {name}")
                    continue
                if not _valid_spirv(data, variant["stage"], variant.get("entry", "main")):
                    problems.append(f"invalid SPIR-V stage/entry: {name}")
    except (BadZipFile, OSError) as error:
        problems.append(f"cannot inspect APK {apk}: {error}")
    return problems


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shader-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--apk", type=Path,
                        help="also verify the final APK contains libmain.so and every shader")
    args = parser.parse_args()
    errors = check_assets(args.shader_root, args.manifest)
    if args.apk:
        errors.extend(check_apk(args.apk, args.manifest))
    for error in errors:
        print(error)
    raise SystemExit(1 if errors else 0)
