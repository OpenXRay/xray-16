#!/usr/bin/env python3
"""Compile local deferred lighting for the Vulkan gameplay renderer."""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/Layers/xrRenderVK/smoke/local_light.frag"
HEADER = ROOT / "src/Layers/xrRenderVK/LocalLightShaders.h"


def generate(glslc):
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "local_light.spv"
        subprocess.run([glslc, "--target-env=vulkan1.0", "-o", str(output), str(SOURCE)],
                       check=True, capture_output=True)
        data = output.read_bytes()
    if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
        raise ValueError("invalid local light SPIR-V")
    words = struct.unpack(f"<{len(data) // 4}I", data)
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "namespace xray::render::vulkan::local_light_shaders", "{",
             "inline constexpr uint32_t Fragment[] = {"]
    for start in range(0, len(words), 8):
        lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
    return "\n".join(lines + ["};", "}", ""])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = generate(args.glslc)
    if args.check:
        if HEADER.read_text() != result:
            parser.error("LocalLightShaders.h is stale")
    else:
        HEADER.write_text(result)
