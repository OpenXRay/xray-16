import importlib.util
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools/generate_vulkan_alpha_test_shader.py"
OUTPUT = ROOT / "src/Layers/xrRenderVK/DeferredAlphaTestShaders.h"


class VulkanAlphaTestShaderTest(unittest.TestCase):
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
        self.assertEqual(words[3], 41)
        opcodes = []
        offset = 5
        while offset < len(words):
            word_count = words[offset] >> 16
            self.assertGreater(word_count, 0)
            self.assertLessEqual(offset + word_count, len(words))
            opcodes.append(words[offset] & 0xFFFF)
            offset += word_count
        self.assertEqual(offset, len(words))
        self.assertIn(184, opcodes)  # OpFOrdLessThan(alpha, 0.5)
        self.assertIn(247, opcodes)  # OpSelectionMerge
        self.assertIn(250, opcodes)  # OpBranchConditional
        self.assertIn(252, opcodes)  # OpKill


if __name__ == "__main__":
    unittest.main()
