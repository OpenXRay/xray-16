#!/usr/bin/env python3
"""Embed validated cutout and transparent gameplay shaders in xrRenderVK."""

import argparse
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
SOURCES = (("level_cutout.ps.spv", "CutoutFragment"),
           ("hud_blended.ps.spv", "TransparentFragment"))
HEADER = ROOT / "src/Layers/xrRenderVK/VulkanGameShaders.h"


def words_for(data):
    if len(data) < 20 or len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
        raise ValueError("invalid game SPIR-V binary")
    return struct.unpack(f"<{len(data) // 4}I", data)


def generate():
    lines = ["#pragma once", "", "#include <cstdint>", "",
             "namespace xray::render::vulkan::game_shaders", "{"]
    for filename, name in SOURCES:
        words = words_for((ROOT / "res/gamedata/shaders/vk" / filename).read_bytes())
        lines.append(f"inline constexpr uint32_t {name}[] = {{")
        for start in range(0, len(words), 8):
            lines.append("    " + ", ".join(f"0x{word:08x}u" for word in words[start:start + 8]) + ",")
        lines.extend(["};", ""])
    return "\n".join(lines + ["}", ""])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate()
    if args.check:
        if HEADER.read_text() != generated:
            parser.error("VulkanGameShaders.h is stale")
    else:
        HEADER.write_text(generated)
