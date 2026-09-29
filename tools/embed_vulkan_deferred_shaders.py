#!/usr/bin/env python3
"""Compile the Vulkan G-buffer and fullscreen light shaders into a C++ header."""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "src/Layers/xrRenderVK/smoke"
HEADER = ROOT / "src/Layers/xrRenderVK/DeferredShaders.h"


def generate(glslc):
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "// Generated from smoke/gbuffer.* and deferred.* for Vulkan 1.0.",
             "namespace xray::render::vulkan::deferred_shaders", "{"]
    with tempfile.TemporaryDirectory() as directory:
        for source, name in (("gbuffer.vert", "GBufferVertex"),
                             ("gbuffer.frag", "GBufferFragment"),
                             ("deferred.vert", "LightVertex"),
                             ("deferred.frag", "LightFragment")):
            target = Path(directory) / (source + ".spv")
            subprocess.run([glslc, "--target-env=vulkan1.0", "-o", str(target),
                            str(SOURCES / source)], check=True, capture_output=True)
            data = target.read_bytes()
            if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
                raise ValueError(f"invalid SPIR-V from {source}")
            words = struct.unpack("<" + "I" * (len(data) // 4), data)
            lines.append(f"inline constexpr uint32_t {name}[] = {{")
            for start in range(0, len(words), 8):
                lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
            lines.extend(["};", ""])
    lines.extend(["}", ""])
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate(args.glslc)
    if args.check:
        if HEADER.read_text() != generated:
            parser.error("DeferredShaders.h is stale")
    else:
        HEADER.write_text(generated)
