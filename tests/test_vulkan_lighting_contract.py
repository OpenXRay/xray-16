"""Validate the compiled fullscreen lighting paths and their shared UBO ABI."""

from pathlib import Path
import re
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]


def instructions(path):
    data = path.read_bytes()
    words = struct.unpack(f"<{len(data) // 4}I", data)
    if len(data) < 20 or len(data) % 4 or words[0] != 0x07230203:
        raise ValueError("invalid SPIR-V")
    position = 5
    while position < len(words):
        count, opcode = words[position] >> 16, words[position] & 0xffff
        if not count or position + count > len(words):
            raise ValueError("invalid instruction")
        yield opcode, words[position + 1:position + count]
        position += count


def embedded_shader(header, name):
    content = (ROOT / "src/Layers/xrRenderVK" / header).read_text()
    match = re.search(rf"inline constexpr uint32_t {name}\[\] = \{{(.*?)\}};", content, re.S)
    if not match:
        raise ValueError(f"missing {name} in {header}")
    words = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]+)u", match[1])]
    if words[0] != 0x07230203:
        raise ValueError("invalid embedded SPIR-V")
    position = 5
    while position < len(words):
        count, opcode = words[position] >> 16, words[position] & 0xffff
        if not count or position + count > len(words):
            raise ValueError("invalid embedded instruction")
        yield opcode, words[position + 1:position + count]
        position += count


def named_struct_offsets(code, name):
    names = {args[0]: struct.pack(f"<{len(args) - 1}I", *args[1:]).split(b"\0")[0]
             for op, args in code if op == 5}
    identifier = next(key for key, value in names.items() if value == name.encode())
    return {args[1]: args[3] for op, args in code
            if op == 72 and len(args) >= 4 and args[0] == identifier and args[2] == 35}


class LightingContractTest(unittest.TestCase):
    def test_plain_and_weather_read_surface_and_rgb_environment(self):
        for header, shader in (("DeferredShaders.h", "LightFragment"),
                               ("WeatherShaders.h", "Fragment")):
            with self.subTest(header=header):
                code = list(embedded_shader(header, shader))
                offsets = named_struct_offsets(code, "ShadowConstants")
                self.assertEqual(offsets, {0: 0, 1: 64, 2: 128, 3: 144, 4: 160})
                decorated = {(args[0], args[1]): args[2] for op, args in code
                             if op == 71 and len(args) >= 3}
                self.assertTrue(any(decorated.get((key, 33)) == 5 and value == 0
                                    for (key, kind), value in decorated.items() if kind == 34))

    def test_forward_rgb_ambient_hemi_and_local_light_offsets(self):
        for shader in ("object_blended", "skinned_blended", "level_detail_blended"):
            with self.subTest(shader=shader):
                code = list(instructions(ROOT / "res/gamedata/shaders/vk" /
                                         f"{shader}.ps.spv"))
                offsets = named_struct_offsets(code, "ForwardLighting")
                self.assertEqual(offsets, {0: 0, 1: 64, 2: 80, 3: 96, 4: 112,
                                           5: 128, 6: 144, 7: 160, 8: 176})


if __name__ == "__main__":
    unittest.main()
