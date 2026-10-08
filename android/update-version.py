#!/usr/bin/env python3
"""Keep the Android version file in sync with commits after the port baseline."""
import argparse
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
VERSION_FILE = ROOT / "android/PORT_VERSION"


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def expected():
    baseline = git("log", "-1", "--format=%H", "--", "android/PORT_VERSION")
    if not baseline:
        raise RuntimeError("Android version file is absent from this branch")
    lines = git("show", f"{baseline}:android/PORT_VERSION").splitlines()
    major, minor, patch = map(int, lines[0].split("."))
    code = int(lines[1]) if len(lines) > 1 else 40
    subjects = git("log", "--first-parent", "--format=%s", f"{baseline}..HEAD").splitlines()
    commits = sum(not subject.startswith("chore(android): bump version") for subject in subjects)
    return f"{major}.{minor}.{patch + commits}\n{code + commits}\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--pending", action="store_true", help="include the commit being prepared")
    args = parser.parse_args()
    content = expected()
    if args.pending:
        lines = content.splitlines()
        major, minor, patch = map(int, lines[0].split("."))
        content = f"{major}.{minor}.{patch + 1}\n{int(lines[1]) + 1}\n"
    if args.check:
        if VERSION_FILE.read_text() != content:
            parser.error(f"Android version is stale; expected {content.splitlines()[0]}")
    else:
        VERSION_FILE.write_text(content)
    print(content.splitlines()[0])


if __name__ == "__main__":
    main()
