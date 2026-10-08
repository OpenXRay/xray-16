import importlib.util
import contextlib
import io
import json
import struct
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "compile_vulkan_shader.py"
spec = importlib.util.spec_from_file_location("compile_vulkan_shader", SCRIPT)
compiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compiler)


class CompileVulkanShaderTest(unittest.TestCase):
    @staticmethod
    def fake_dxc(path, body=""):
        path.write_text("#!/usr/bin/env python3\nimport sys, struct\nfrom pathlib import Path\n"
                        "args = sys.argv[1:]\n" + body +
                        "model = 0 if args[args.index('-T') + 1].startswith('vs') else 4\n"
                        "words = [0x07230203, 0x10000, 0, 2, 0, 0x0005000f, "
                        "model, 1, 0x6e69616d, 0]\n"
                        "Path(args[args.index('-Fo') + 1]).write_bytes(struct.pack('<10I', *words))\n")
        path.chmod(0o755)

    def test_compile_preserves_previous_output_on_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "game" / "shader.ps"
            source.parent.mkdir()
            source.write_text("float4 main() : SV_Target { return 1; }")
            output = root / "cache" / "shader.ps.spv"
            dxc = root / "fake-dxc"
            self.fake_dxc(dxc)
            args = ["--dxc", str(dxc), "--source", str(source), "--output", str(output),
                    "--stage", "ps", "--entry", "main", "--define", "TEST=1"]
            self.assertEqual(compiler.compile_shader(args), 0)
            self.assertEqual(output.read_bytes()[:4], bytes.fromhex("03022307"))
            dxc.write_text("#!/usr/bin/env python3\nimport sys\nsys.exit(3)\n")
            self.assertEqual(compiler.compile_shader(args), 3)
            self.assertEqual(output.read_bytes()[:4], bytes.fromhex("03022307"))

    def test_variant_set_is_atomic_and_reports_failing_variant(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "a.vs").write_text("float4 main() : SV_Position { return 0; }")
            (root / "b.ps").write_text("float4 main() : SV_Target { return 1; }")
            manifest = root / "variants.json"
            manifest.write_text(json.dumps({"include": ["."], "variants": [
                {"source": "a.vs", "stage": "vs", "output": "game/a.vs.spv",
                 "define": ["SKIN_NONE=1"]},
                {"source": "b.ps", "stage": "ps", "output": "game/b.ps.spv",
                 "define": ["USE_AREF=1"]}]}))
            output_dir = root / "compiled"
            old = output_dir / "game" / "a.vs.spv"
            old.parent.mkdir(parents=True)
            old.write_bytes(b"old")
            dxc = root / "fake-dxc"
            self.fake_dxc(dxc,
                "if any('b.ps' in x for x in args) and Path(__file__).with_suffix('.fail').exists():\n"
                " print('bad macro', file=sys.stderr); sys.exit(4)\n"
                "Path(__file__).with_suffix('.calls').open('a').write(' '.join(args)+'\\n')\n")
            args = ["--dxc", str(dxc), "--manifest", str(manifest),
                    "--output-dir", str(output_dir)]
            dxc.with_suffix(".fail").touch()
            errors = io.StringIO()
            with contextlib.redirect_stderr(errors):
                self.assertEqual(compiler.compile_shader(args), 4)
            self.assertIn("b.ps [ps, USE_AREF=1]", errors.getvalue())
            self.assertIn("bad macro", errors.getvalue())
            self.assertEqual(old.read_bytes(), b"old")
            self.assertFalse((output_dir / "game" / "b.ps.spv").exists())
            dxc.with_suffix(".fail").unlink()
            self.assertEqual(compiler.compile_shader(args), 0)
            self.assertEqual(old.read_bytes()[:4], bytes.fromhex("03022307"))
            self.assertIn("-D SKIN_NONE=1", dxc.with_suffix(".calls").read_text())
            self.assertTrue((output_dir / "game" / "b.ps.spv").exists())

    def test_wrong_stage_or_entry_is_rejected_without_overwriting(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "game.ps"
            source.write_text("float4 main() : SV_Target { return 1; }")
            output = root / "game.ps.spv"
            output.write_bytes(b"previous shader")
            dxc = root / "fake-dxc"
            self.fake_dxc(dxc)
            errors = io.StringIO()
            with contextlib.redirect_stderr(errors):
                self.assertEqual(compiler.compile_shader([
                    "--dxc", str(dxc), "--source", str(source), "--output", str(output),
                    "--stage", "ps", "--entry", "not_main"]), 1)
            self.assertIn("missing ps entry 'not_main'", errors.getvalue())
            self.assertEqual(output.read_bytes(), b"previous shader")


if __name__ == "__main__":
    unittest.main()
