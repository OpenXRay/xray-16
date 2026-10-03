#!/usr/bin/env python3
"""Derive the fixed-cutoff alpha-test SPIR-V variant from DeferredShaders.h."""

import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/Layers/xrRenderVK/DeferredShaders.h"
GLSL_SOURCE = ROOT / "src/Layers/xrRenderVK/smoke/gbuffer_alpha_test.frag"
OUTPUT = ROOT / "src/Layers/xrRenderVK/DeferredAlphaTestShaders.h"


def instructions(words):
    result = []
    cursor = 5
    while cursor < len(words):
        count = words[cursor] >> 16
        if count == 0 or cursor + count > len(words):
            raise ValueError("malformed SPIR-V instruction stream")
        result.append((cursor, words[cursor] & 0xFFFF, words[cursor + 1:cursor + count]))
        cursor += count
    if cursor != len(words):
        raise ValueError("trailing words in SPIR-V module")
    return result


def derive(base):
    if len(base) < 5 or base[0] != 0x07230203:
        raise ValueError("base fragment shader is not SPIR-V")
    rows = instructions(base)
    float_type = next((args[0] for _, op, args in rows if op == 22 and args[1] == 32), None)
    sample = next(((offset, args) for offset, op, args in rows if op == 87), None)
    if float_type is None or sample is None:
        raise ValueError("base fragment shader no longer has the expected texture sample")

    # Insert OpTypeBool before pointers and the float constant before functions.
    bool_type = base[3]
    cutoff = bool_type + 1
    alpha = bool_type + 2
    condition = bool_type + 3
    discard_label = bool_type + 4
    merge_label = bool_type + 5
    insert_type = next(offset for offset, op, _ in rows if op == 32)
    insert_function = next(offset for offset, op, _ in rows if op == 54)
    result = list(base)
    result[3] += 6
    result[insert_type:insert_type] = [(2 << 16) | 20, bool_type]
    # Insertion shifts all later offsets, so locate the function boundary again.
    rows = instructions(result)
    insert_function = next(offset for offset, op, _ in rows if op == 54)
    result[insert_function:insert_function] = [
        (4 << 16) | 43, float_type, cutoff, 0x3F000000,
    ]

    # Add alpha extraction, ordered comparison and a structured discard branch
    # directly after the texture sample and before its color output is stored.
    rows = instructions(result)
    sample_row = next((row for row in rows if row[1] == 87), None)
    sample_offset, _, sample_args = sample_row
    sampled_color = sample_args[1]
    insert_after_sample = sample_offset + (result[sample_offset] >> 16)
    body = [
        (5 << 16) | 81, float_type, alpha, sampled_color, 3,
        (5 << 16) | 184, bool_type, condition, alpha, cutoff,
        (3 << 16) | 247, merge_label, 0,
        (4 << 16) | 250, condition, discard_label, merge_label,
        (2 << 16) | 248, discard_label,
        (1 << 16) | 252,
        (2 << 16) | 248, merge_label,
    ]
    result[insert_after_sample:insert_after_sample] = body
    return result


def render(words):
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "// Derived from DeferredShaders.h with a 0.5 diffuse-alpha discard.",
             "namespace xray::render::vulkan::deferred_shaders", "{",
             "inline constexpr uint32_t GBufferAlphaTestFragment[] = {"]
    for start in range(0, len(words), 8):
        lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
    lines.extend(["};", "}", ""])
    return "\n".join(lines)


def generate():
    glsl = GLSL_SOURCE.read_text()
    if not re.search(r"if\s*\(\s*albedo\.a\s*<\s*0\.5\s*\)\s*discard\s*;", glsl):
        raise ValueError("alpha-test GLSL source must discard diffuse alpha below 0.5")
    text = SOURCE.read_text()
    match = re.search(r"inline constexpr uint32_t GBufferFragment\[\] = \{(.*?)\};", text, re.S)
    if not match:
        raise ValueError("GBufferFragment array not found")
    base = [int(word, 16) for word in re.findall(r"0x([0-9a-fA-F]+)u", match.group(1))]
    return render(derive(base))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != generated:
            parser.error("DeferredAlphaTestShaders.h is stale")
    else:
        OUTPUT.write_text(generated)
