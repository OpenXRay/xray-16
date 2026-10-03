#!/usr/bin/env python3
"""Fail APK staging when a declared Vulkan shader binary is missing or invalid."""

import argparse
import json
from pathlib import Path

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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shader-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()
    errors = check_assets(args.shader_root, args.manifest)
    for error in errors:
        print(error)
    raise SystemExit(1 if errors else 0)
