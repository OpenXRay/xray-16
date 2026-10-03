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
import struct
import sys
import tempfile

STAGES = ("vs", "ps", "cs", "gs", "hs", "ds")
SPIRV_MAGIC = b"\x03\x02\x23\x07"
EXECUTION_MODEL = {"vs": 0, "hs": 1, "ds": 2, "gs": 3, "ps": 4, "cs": 5}


def _valid_spirv(data, stage, entry):
    if len(data) < 20 or len(data) % 4 or data[:4] != SPIRV_MAGIC:
        return False
    words = struct.unpack(f"<{len(data) // 4}I", data)
    if words[3] == 0 or words[4] != 0:
        return False
    cursor = 5
    found = False
    while cursor < len(words):
        instruction = words[cursor]
        size, opcode = instruction >> 16, instruction & 0xFFFF
        if size == 0 or cursor + size > len(words):
            return False
        if opcode == 15 and size >= 4:
            name = struct.pack(f"<{size - 3}I", *words[cursor + 3:cursor + size]).split(b"\0", 1)[0]
            found |= words[cursor + 1] == EXECUTION_MODEL[stage] and name == entry.encode("utf-8")
        cursor += size
    return found


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


def _compile(variant, dxc, temporary, compiler="dxc"):
    source = variant["source"]
    if compiler == "glslc":
        stage = {"vs": "vertex", "ps": "fragment", "cs": "compute",
                 "gs": "geometry", "hs": "tesscontrol", "ds": "tesseval"}[variant["stage"]]
        command = [dxc, "-x", "hlsl", "-fauto-combined-image-sampler", f"-fshader-stage={stage}",
                   f"-fentry-point={variant['entry']}", "--target-env=vulkan1.0",
                   "-o", str(temporary), str(source)]
        for directory in [source.parent, *variant["includes"]]:
            command += ["-I", str(directory)]
        for definition in variant["defines"]:
            command += [f"-D{definition}"]
    else:
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
        print(f"Shader compiler not found: {dxc}", file=sys.stderr)
        return 1
    if result.returncode:
        print(f"Shader compilation failed for {source} [{variant['stage']}, "
              f"{', '.join(variant['defines']) or 'default'}] (exit {result.returncode}):\n"
              f"{result.stdout}{result.stderr}", file=sys.stderr)
        return result.returncode
    data = temporary.read_bytes()
    if not _valid_spirv(data, variant["stage"], variant["entry"]):
        print(f"Compiler produced invalid SPIR-V or missing {variant['stage']} entry "
              f"'{variant['entry']}' for {source}", file=sys.stderr)
        return 1
    return 0


def compile_shader(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dxc", default=os.environ.get("DXC", "dxc"))
    parser.add_argument("--compiler", choices=("dxc", "glslc"), default="dxc")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--stage", choices=STAGES)
    parser.add_argument("--entry")
    parser.add_argument("--include", action="append", default=[], type=Path)
    parser.add_argument("--define", action="append", default=[])
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args(argv)
    if args.compiler == "glslc" and args.dxc == "dxc":
        args.dxc = "glslc"
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
            status = _compile(variant, args.dxc, temporary, args.compiler)
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
