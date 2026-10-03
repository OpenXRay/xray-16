"""Exercise the real commit hook in disposable repositories."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class VersionHookTest(unittest.TestCase):
    def test_commit_retry_and_partial_staging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            def git(*args):
                return subprocess.check_output(['git', '-C', directory, *args], text=True).strip()
            git('init', '-q')
            git('config', 'user.name', 'Version test')
            git('config', 'user.email', 'test@example.invalid')
            (root / 'android').mkdir()
            shutil.copy(ROOT / 'android/update-version.py', root / 'android/update-version.py')
            shutil.copytree(ROOT / '.githooks', root / '.githooks')
            (root / 'android/PORT_VERSION').write_text('0.9.29\n43\n')
            git('add', '.')
            git('commit', '-qm', 'baseline')
            git('config', 'core.hooksPath', '.githooks')
            (root / 'feature').write_text('staged\n')
            git('add', 'feature')
            (root / 'feature').write_text('unstaged\n')
            # An aborted commit/retry must derive the same version from HEAD.
            subprocess.check_call([str(root / '.githooks/pre-commit')], cwd=root)
            subprocess.check_call([str(root / '.githooks/pre-commit')], cwd=root)
            self.assertEqual((root / 'android/PORT_VERSION').read_text(), '0.9.30\n44\n')
            git('commit', '-qm', 'feature')
            self.assertEqual(git('show', 'HEAD:android/PORT_VERSION'), '0.9.30\n44')
            self.assertEqual(git('show', 'HEAD:feature'), 'staged')
            self.assertEqual((root / 'feature').read_text(), 'unstaged\n')
            git('add', 'feature')
            git('commit', '-qm', 'second change')
            self.assertEqual(git('show', 'HEAD:android/PORT_VERSION'), '0.9.31\n45')
            subprocess.check_call(['python3', str(root / 'android/update-version.py'), '--check'])


if __name__ == '__main__':
    unittest.main()
