import importlib.util
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools/generate_vulkan_alpha_test_shader.py"
OUTPUT = ROOT / "src/Layers/xrRenderVK/DeferredAlphaTestShaders.h"


def shader_interface(header, name, storage):
    match = re.search(rf"{name}\[\] = \{{(.*?)\}};", header.read_text(), re.S)
    if not match:
        raise ValueError(f"missing embedded shader: {name}")
    words = [int(word, 16) for word in re.findall(r"0x([0-9a-fA-F]+)u", match.group(1))]
    locations = {}
    variables = set()
    offset = 5
    while offset < len(words):
        count = words[offset] >> 16
        opcode = words[offset] & 0xffff
        args = words[offset + 1:offset + count]
        if opcode == 71 and len(args) >= 3 and args[1] == 30:  # OpDecorate Location
            locations[args[0]] = args[2]
        if opcode == 59 and len(args) >= 3 and args[2] == storage:  # OpVariable
            variables.add(args[1])
        offset += count
    return {locations[identifier] for identifier in variables if identifier in locations}


class VulkanAlphaTestShaderTest(unittest.TestCase):
    def test_deferred_cutout_stage_interface(self):
        vertex = shader_interface(ROOT / "src/Layers/xrRenderVK/DeferredShaders.h", "GBufferVertex", 3)
        fragment = shader_interface(OUTPUT, "GBufferAlphaTestFragment", 1)
        self.assertEqual(fragment, {0, 1})
        self.assertTrue(fragment <= vertex)
        factory = (ROOT / "src/Layers/xrRenderVK/DeferredShaderFactory.cpp").read_text()
        self.assertIn("alpha_test_fragment.initialize(device, shader_dispatch, deferred_shaders::GBufferAlphaTestFragment", factory)

    def test_generated_variant_contains_cutoff_discard(self):
        spec = importlib.util.spec_from_file_location("alpha_shader_generator", SCRIPT)
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        self.assertEqual(OUTPUT.read_text(), generator.generate())

        source = OUTPUT.read_text()
        match = re.search(r"GBufferAlphaTestFragment\[\] = \{(.*?)\};", source, re.S)
        self.assertIsNotNone(match)
        words = [int(word, 16) for word in re.findall(r"0x([0-9a-fA-F]+)u", match.group(1))]
        self.assertEqual(words[0], 0x07230203)
        self.assertGreater(words[3], 41)
        opcodes = []
        member_offsets = set()
        push_constants = 0
        offset = 5
        while offset < len(words):
            word_count = words[offset] >> 16
            self.assertGreater(word_count, 0)
            self.assertLessEqual(offset + word_count, len(words))
            opcodes.append(words[offset] & 0xFFFF)
            args = words[offset + 1:offset + word_count]
            if (words[offset] & 0xFFFF) == 72 and len(args) >= 4 and args[2] == 35:
                member_offsets.add(args[3])
            if (words[offset] & 0xFFFF) == 59 and len(args) >= 3 and args[2] == 9:
                push_constants += 1
            offset += word_count
        self.assertEqual(offset, len(words))
        self.assertIn(64, member_offsets)  # Matrix precedes the material cutoff.
        self.assertEqual(push_constants, 1)
        self.assertIn(247, opcodes)  # OpSelectionMerge
        self.assertIn(250, opcodes)  # OpBranchConditional
        self.assertIn(252, opcodes)  # OpKill


if __name__ == "__main__":
    unittest.main()
