#!/usr/bin/env python3
"""Embed Vulkan sun shadow depth shader variants."""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/Layers/xrRenderVK/smoke"
HEADER = ROOT / "src/Layers/xrRenderVK/ShadowShaders.h"


def generate(compiler):
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "namespace xray::render::vulkan::shadow_shaders", "{"]
    with tempfile.TemporaryDirectory() as temporary:
        for name, file in (("Vertex", "shadow.vert"), ("Opaque", "shadow_opaque.frag"),
                           ("Cutout", "shadow_cutout.frag")):
            output = Path(temporary) / (file + ".spv")
            subprocess.run([compiler, "--target-env=vulkan1.0", "-o", str(output),
                            str(SOURCE / file)], check=True, capture_output=True)
            data = output.read_bytes()
            if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
                raise ValueError(f"invalid shadow SPIR-V: {file}")
            words = struct.unpack(f"<{len(data) // 4}I", data)
            lines.append(f"inline constexpr uint32_t {name}[] = {{")
            for start in range(0, len(words), 8):
                lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
            lines += ["};", ""]
    return "\n".join(lines + ["}", ""])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = generate(args.glslc)
    if args.check:
        if HEADER.read_text() != text:
            parser.error("ShadowShaders.h is stale")
    else:
        HEADER.write_text(text)
