#!/usr/bin/env python3
"""Compile game HLSL variants to SPIR-V with DXC, preserving game sources.

Single shader: --source FILE --stage ps --entry main --output FILE
Variant set:   --manifest FILE --output-dir DIR
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

STAGES = ("vs", "ps", "cs", "gs", "hs", "ds")
SPIRV_MAGIC = b"\x03\x02\x23\x07"


def _variant(source, stage, entry, output, includes, defines):
    if stage not in STAGES or not entry or not source.is_file():
        raise ValueError(f"invalid shader source/stage/entry: {source} ({stage}, {entry})")
    if not isinstance(defines, list) or not all(isinstance(x, str) and x for x in defines):
        raise ValueError(f"invalid defines for {source}")
    return dict(source=source, stage=stage, entry=entry, output=output,
                includes=includes, defines=defines)


def _manifest(path, output_dir):
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or not isinstance(data.get("variants"), list) or not data["variants"]:
        raise ValueError("manifest requires a nonempty variants array")
    root = path.parent.resolve()
    common = data.get("include", [])
    if not isinstance(common, list) or not all(isinstance(x, str) for x in common):
        raise ValueError("manifest include must be an array of paths")
    variants = []
    outputs = set()
    for item in data["variants"]:
        if not isinstance(item, dict) or not isinstance(item.get("output"), str):
            raise ValueError("each variant requires an output path")
        relative = Path(item["output"])
        if relative.is_absolute() or ".." in relative.parts or relative == Path("."):
            raise ValueError(f"variant output must stay inside output-dir: {relative}")
        output = output_dir / relative
        if output in outputs:
            raise ValueError(f"duplicate shader variant output: {relative}")
        outputs.add(output)
        source = root / item["source"]
        includes = [root / x for x in [*common, *item.get("include", [])]]
        variants.append(_variant(source, item["stage"], item.get("entry", "main"),
                                 output, includes, item.get("define", [])))
    return variants


def _compile(variant, dxc, temporary):
    source = variant["source"]
    command = [dxc, "-spirv", "-fspv-target-env=vulkan1.0", "-Zpr",
               "-T", f"{variant['stage']}_6_0", "-E", variant["entry"],
               "-Fo", str(temporary), str(source)]
    for directory in [source.parent, *variant["includes"]]:
        command += ["-I", str(directory)]
    for definition in variant["defines"]:
        command += ["-D", definition]
    try:
        result = subprocess.run(command, check=False, capture_output=True, text=True)
    except FileNotFoundError:
        print(f"DXC executable not found: {dxc}", file=sys.stderr)
        return 1
    if result.returncode:
        print(f"DXC failed for {source} [{variant['stage']}, "
              f"{', '.join(variant['defines']) or 'default'}] (exit {result.returncode}):\n"
              f"{result.stdout}{result.stderr}", file=sys.stderr)
        return result.returncode
    data = temporary.read_bytes()
    if len(data) < 20 or len(data) % 4 or data[:4] != SPIRV_MAGIC:
        print(f"DXC produced invalid SPIR-V for {source} [{variant['stage']}]", file=sys.stderr)
        return 1
    return 0


def compile_shader(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dxc", default=os.environ.get("DXC", "dxc"))
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--stage", choices=STAGES)
    parser.add_argument("--entry")
    parser.add_argument("--include", action="append", default=[], type=Path)
    parser.add_argument("--define", action="append", default=[])
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args(argv)
    if args.manifest:
        if not args.output_dir or any((args.source, args.output, args.stage, args.entry,
                                       args.include, args.define)):
            parser.error("--manifest requires --output-dir and no single-shader options")
        try:
            variants = _manifest(args.manifest, args.output_dir)
        except (OSError, ValueError, KeyError, TypeError) as exc:
            parser.error(f"invalid shader manifest: {exc}")
    else:
        if not args.source or not args.output or not args.stage or not args.entry:
            parser.error("single shader requires --source, --output, --stage and --entry")
        try:
            variants = [_variant(args.source, args.stage, args.entry, args.output,
                                 args.include, args.define)]
        except ValueError as exc:
            parser.error(str(exc))

    # Compile the complete set before replacing any existing output. Failed
    # variants cannot leave a mixture of new and old shader permutations.
    pending = []
    try:
        for variant in variants:
            output = variant["output"]
            output.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(dir=output.parent, suffix=".spv", delete=False) as tmp:
                temporary = Path(tmp.name)
            pending.append((temporary, output))
            status = _compile(variant, args.dxc, temporary)
            if status:
                return status
        for temporary, output in pending:
            temporary.replace(output)
        return 0
    finally:
        for temporary, _ in pending:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    sys.exit(compile_shader())
