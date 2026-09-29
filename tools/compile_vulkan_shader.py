#!/usr/bin/env python3
"""Compile a game/mod HLSL source with DXC for Vulkan, keeping its source tree intact.

Example: python3 tools/compile_vulkan_shader.py --source res/gamedata/shaders/r3/editor.vs \
    --stage vs --entry main --output build/shaders/r3/editor.vs.spv
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def compile_shader(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dxc", default=os.environ.get("DXC", "dxc"))
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--stage", required=True, choices=("vs", "ps", "cs", "gs", "hs", "ds"))
    parser.add_argument("--entry", required=True)
    parser.add_argument("--include", action="append", default=[], type=Path)
    parser.add_argument("--define", action="append", default=[])
    args = parser.parse_args(argv)
    if not args.source.is_file():
        parser.error(f"shader source does not exist: {args.source}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # DXC resolves relative includes from the source folder; additional roots
    # allow a game/mod installation to supply its own common headers.
    with tempfile.NamedTemporaryFile(dir=args.output.parent, suffix=".spv", delete=False) as tmp:
        temporary = Path(tmp.name)
    command = [args.dxc, "-spirv", "-fspv-target-env=vulkan1.0", "-Zpr",
               "-T", f"{args.stage}_6_0", "-E", args.entry,
               "-Fo", str(temporary), str(args.source)]
    for directory in [args.source.parent, *args.include]:
        command += ["-I", str(directory)]
    for definition in args.define:
        command += ["-D", definition]
    try:
        result = subprocess.run(command, check=False)
        if result.returncode:
            return result.returncode
        data = temporary.read_bytes()
        if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
            print("DXC did not produce valid SPIR-V", file=sys.stderr)
            return 1
        temporary.replace(args.output)
        return 0
    except FileNotFoundError:
        print(f"DXC executable not found: {args.dxc}", file=sys.stderr)
        return 1
    finally:
        temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    sys.exit(compile_shader())
