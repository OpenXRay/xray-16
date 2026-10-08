"""Check the shader binaries against the Vulkan game pipeline interfaces."""

import json
from pathlib import Path
import struct
import tempfile
import unittest
from zipfile import ZipFile

from tools.check_vulkan_shader_assets import check_apk


ROOT = Path(__file__).resolve().parents[1]
SHADERS = ROOT / "res/gamedata/shaders"
MANIFEST = SHADERS / "vk/opaque-variants.json"


def instructions(path):
    data = path.read_bytes()
    if len(data) < 20 or len(data) % 4:
        raise ValueError(f"invalid SPIR-V length: {path}")
    words = struct.unpack(f"<{len(data) // 4}I", data)
    if words[0] != 0x07230203:
        raise ValueError(f"invalid SPIR-V magic: {path}")
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 0xffff
        if not count or offset + count > len(words):
            raise ValueError(f"invalid instruction: {path}")
        yield opcode, words[offset + 1:offset + count]
        offset += count


class GameShaderVariantsTest(unittest.TestCase):
    def test_manifest_entries_and_pipeline_interfaces(self):
        variants = json.loads(MANIFEST.read_text())["variants"]
        outputs = {v["output"]: v for v in variants}
        self.assertEqual(len(outputs), len(variants))
        expected = {
            "vk/level_opaque.vs.spv", "vk/level_opaque.ps.spv",
            "vk/object_opaque.vs.spv", "vk/object_opaque.ps.spv",
            "vk/level_cutout.ps.spv", "vk/object_cutout.ps.spv",
            "vk/object_blended.ps.spv", "vk/skinned_blended.ps.spv",
            "vk/hud_blended.ps.spv", "vk/object_double_sided.ps.spv",
            *{f"vk/skinned_{n}.vs.spv" for n in range(1, 5)},
            "vk/hud_skinned_1.vs.spv", "vk/hud_skinned_2.vs.spv",
            "vk/hud_skinned_3.vs.spv", "vk/hud_skinned_4.vs.spv", "vk/tree_opaque.vs.spv",
            "vk/progressive_opaque.vs.spv",
        }
        self.assertTrue(expected <= outputs.keys())
        for name, variant in outputs.items():
            with self.subTest(name=name):
                code = list(instructions(SHADERS / name))
                model = 0 if variant["stage"] == "vs" else 4
                self.assertTrue(any(op == 15 and args[0] == model for op, args in code))
                decorations = {(args[0], args[1]): args[2] for op, args in code
                               if op == 71 and len(args) >= 3}
                bindings = {identifier: value for (identifier, kind), value in decorations.items() if kind == 33}
                outputs_ids = {args[1] for op, args in code if op == 59 and args[2] == 3}
                output_locations = {decorations[(identifier, 30)] for identifier in outputs_ids
                                    if (identifier, 30) in decorations}
                locations = {value for (identifier, kind), value in decorations.items() if kind == 30}
                if name.endswith(".ps.spv"):
                    self.assertIn(0, output_locations)
                    self.assertIn(0, bindings.values())
                    if "blended" in name or "particle_" in name or "alpha_add" in name:
                        # The present pass has one color attachment, unlike the G-buffer.
                        self.assertEqual(output_locations, {0})
                    else:
                        self.assertEqual(output_locations, {0, 1})
                if "skinned" in name and name.endswith(".vs.spv"):
                    self.assertTrue({0, 1, 2, 3, 4} <= locations)
                    self.assertIn(0, bindings.values())
                    self.assertIn(1, {value for (identifier, kind), value in decorations.items()
                                      if kind == 34})  # descriptor set for pose

    def test_cutout_discards_but_blended_does_not(self):
        for variant in ("level_cutout", "object_cutout"):
            code = list(instructions(SHADERS / f"vk/{variant}.ps.spv"))
            opcodes = {op for op, _ in code}
            self.assertIn(252, opcodes)  # OpKill
            offsets = {args[3] for op, args in code
                       if op == 72 and len(args) >= 4 and args[2] == 35}
            self.assertIn(64, offsets)  # The material cutoff follows the MVP.
        blended = {op for op, _ in instructions(SHADERS / "vk/object_blended.ps.spv")}
        self.assertNotIn(252, blended)
        for name in ("particle_world_set", "particle_hud_set"):
            code = list(instructions(SHADERS / f"vk/{name}.ps.spv"))
            self.assertIn(252, {op for op, _ in code})
            offsets = {args[3] for op, args in code
                       if op == 72 and len(args) >= 4 and args[2] == 35}
            self.assertIn(64, offsets)

    def test_forward_transparent_layouts(self):
        for name, expected_set in (("object_blended", 1), ("skinned_blended", 2)):
            code = list(instructions(SHADERS / f"vk/{name}.ps.spv"))
            sets = {args[2] for op, args in code if op == 71 and len(args) >= 3 and args[1] == 34}
            self.assertIn(expected_set, sets)
            offsets = {args[3] for op, args in code if op == 72 and len(args) >= 4 and args[2] == 35}
            self.assertTrue({0, 64, 80, 96, 112} <= offsets)
        hud = list(instructions(SHADERS / "vk/hud_blended.ps.spv"))
        self.assertFalse(any(op == 71 and len(args) >= 3 and args[1] == 34 and args[2] != 0
                             for op, args in hud))

    def test_apk_contains_every_game_pipeline_asset(self):
        variants = json.loads(MANIFEST.read_text())["variants"]
        with tempfile.TemporaryDirectory() as directory:
            apk = Path(directory) / "game.apk"
            with ZipFile(apk, "w") as archive:
                archive.writestr("lib/armeabi-v7a/libmain.so", b"test native library")
                for variant in variants:
                    archive.write(SHADERS / variant["output"],
                                  "assets/gamedata/shaders/" + variant["output"])
            self.assertEqual(check_apk(apk, MANIFEST), [])
            with ZipFile(apk, "w") as archive:
                archive.writestr("lib/armeabi-v7a/libmain.so", b"test native library")
            self.assertIn("missing assets/gamedata/shaders/vk/level_opaque.vs.spv",
                          check_apk(apk, MANIFEST))


if __name__ == "__main__":
    unittest.main()
