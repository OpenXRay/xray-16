#!/usr/bin/env python3
"""Build the Vulkan weather/fog fragment and embed its validated SPIR-V."""

import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/Layers/xrRenderVK/smoke/weather.frag"
HEADER = ROOT / "src/Layers/xrRenderVK/WeatherShaders.h"


def generate(compiler):
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "weather.spv"
        subprocess.run([compiler, "--target-env=vulkan1.0", "-o", str(output),
                        str(SOURCE)], check=True, capture_output=True)
        data = output.read_bytes()
    if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
        raise ValueError("invalid weather SPIR-V")
    words = struct.unpack(f"<{len(data) // 4}I", data)
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "namespace xray::render::vulkan::weather_shaders", "{",
             "inline constexpr uint32_t Fragment[] = {"]
    for start in range(0, len(words), 8):
        lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
    lines += ["};", "}", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glslc", default="glslc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    content = generate(args.glslc)
    if args.check:
        if HEADER.read_text() != content:
            parser.error("WeatherShaders.h is stale")
    else:
        HEADER.write_text(content)
