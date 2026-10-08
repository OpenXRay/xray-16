#!/usr/bin/env python3
"""Compile the G-buffer cutout shader with its per-material alpha threshold."""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GLSL_SOURCE = ROOT / "src/Layers/xrRenderVK/smoke/gbuffer_alpha_test.frag"
OUTPUT = ROOT / "src/Layers/xrRenderVK/DeferredAlphaTestShaders.h"


def generate(glslc="glslc"):
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "cutout.spv"
        subprocess.run([glslc, "--target-env=vulkan1.0", "-o", str(output),
                        str(GLSL_SOURCE)], check=True, capture_output=True)
        data = output.read_bytes()
    if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
        raise ValueError("invalid G-buffer cutout SPIR-V")
    words = struct.unpack("<" + "I" * (len(data) // 4), data)
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "// Generated from smoke/gbuffer_alpha_test.frag for Vulkan 1.0.",
             "namespace xray::render::vulkan::deferred_shaders", "{",
             "inline constexpr uint32_t GBufferAlphaTestFragment[] = {"]
    for start in range(0, len(words), 8):
        lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
    lines.extend(["};", "}", ""])
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate(args.glslc)
    if args.check:
        if OUTPUT.read_text() != generated:
            parser.error("DeferredAlphaTestShaders.h is stale")
    else:
        OUTPUT.write_text(generated)
