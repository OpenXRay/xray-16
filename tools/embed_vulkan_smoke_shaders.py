#!/usr/bin/env python3
"""Regenerate the dependency-free Vulkan smoke shader header with glslang.

Usage: python3 tools/embed_vulkan_smoke_shaders.py --glslang glslangValidator
"""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SHADERS = ROOT / "src/Layers/xrRenderVK/smoke"
HEADER = ROOT / "src/Layers/xrRenderVK/SmokeShaders.h"


def generate(glslang):
    blocks = ["#pragma once", "", "#include <cstdint>", "",
              "// Generated from smoke/triangle.vert and .frag with glslang -V --target-env vulkan1.0.",
              "namespace xray::render::vulkan::smoke", "{"]
    with tempfile.TemporaryDirectory() as folder:
        for stage, name in (("vert", "TriangleVertex"), ("frag", "TriangleFragment")):
            output = Path(folder) / ("triangle." + stage + ".spv")
            subprocess.run([glslang, "-V", "--target-env", "vulkan1.0", "-S", stage,
                            "-o", str(output), str(SHADERS / ("triangle." + stage))], check=True)
            data = output.read_bytes()
            if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
                raise ValueError("glslang returned invalid SPIR-V")
            words = struct.unpack("<" + "I" * (len(data) // 4), data)
            blocks.append(f"inline constexpr uint32_t {name}[] = {{")
            for offset in range(0, len(words), 8):
                blocks.append("    " + ", ".join(f"0x{word:08x}u" for word in words[offset:offset + 8]) + ",")
            blocks += ["};", ""]
    blocks += ["}", ""]
    HEADER.write_text("\n".join(blocks))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslang", default="glslangValidator")
    generate(parser.parse_args().glslang)
