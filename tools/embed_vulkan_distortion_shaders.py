#!/usr/bin/env python3
"""Compile the particle distortion stages used with the scene snapshot."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src/Layers/xrRenderVK/DistortionShaders.h"
def generate(glslc):
    lines = ["#pragma once", "", "#include <cstdint>", "", "namespace xray::render::vulkan::distortion_shaders", "{"]
    with tempfile.TemporaryDirectory() as directory:
        for stage, name in (("vert", "Vertex"), ("frag", "Fragment")):
            output = Path(directory) / (stage + ".spv")
            subprocess.run([glslc, "--target-env=vulkan1.0", "-o", str(output), str(ROOT / ("src/Layers/xrRenderVK/smoke/distortion." + stage))], check=True)
            data = output.read_bytes()
            if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
                raise ValueError("invalid distortion SPIR-V")
            words = struct.unpack(f"<{len(data)//4}I", data)
            lines.append("inline constexpr uint32_t " + name + "[] = {")
            for start in range(0, len(words), 8):
                lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start+8]) + ",")
            lines.append("};")
    return "\n".join(lines + ["}", ""])
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate(args.glslc)
    if args.check:
        if HEADER.read_text() != generated: parser.error("DistortionShaders.h is stale")
    else: HEADER.write_text(generated)
