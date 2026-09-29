import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "compile_vulkan_shader.py"
spec = importlib.util.spec_from_file_location("compile_vulkan_shader", SCRIPT)
compiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compiler)


class CompileVulkanShaderTest(unittest.TestCase):
    def test_compile_preserves_previous_output_on_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "game" / "shader.ps"
            source.parent.mkdir()
            source.write_text("float4 main() : SV_Target { return 1; }")
            output = root / "cache" / "shader.ps.spv"
            dxc = root / "fake-dxc"
            dxc.write_text("#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\n"
                           "args = sys.argv[1:]\n"
                           "Path(args[args.index('-Fo') + 1]).write_bytes(bytes.fromhex('03022307') + bytes(16))\n")
            dxc.chmod(0o755)
            args = ["--dxc", str(dxc), "--source", str(source), "--output", str(output),
                    "--stage", "ps", "--entry", "main", "--define", "TEST=1"]
            self.assertEqual(compiler.compile_shader(args), 0)
            self.assertEqual(output.read_bytes()[:4], bytes.fromhex("03022307"))
            dxc.write_text("#!/usr/bin/env python3\nimport sys\nsys.exit(3)\n")
            self.assertEqual(compiler.compile_shader(args), 3)
            self.assertEqual(output.read_bytes()[:4], bytes.fromhex("03022307"))


if __name__ == "__main__":
    unittest.main()
