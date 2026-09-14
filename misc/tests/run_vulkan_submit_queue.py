import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description="Exercise async Vulkan queue backpressure using a built Ninja xr_3da target and VK_EXT_headless_surface.")
    parser.add_argument("--build-dir", type=Path, default=root / "cmake_builds/releasemastergold")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    database = json.loads((build / "compile_commands.json").read_text())
    entries = [entry for entry in database if Path(entry["file"]).name == "VulkanBackend.cpp"]
    if not entries:
        entries = [entry for entry in database if "/xrRender/" in entry["file"] and "/Unity/" in entry["file"]]
    if not entries:
        parser.error("The build has no VulkanBackend compile command")
    entry = entries[0]
    commands = subprocess.run([
        "ninja", "-C", str(build), "-t", "commands", "xr_3da",
    ], check=True, capture_output=True, text=True).stdout
    link = None
    for line in commands.splitlines():
        for segment in line.split("&&"):
            command = shlex.split(segment)
            if "-o" in command and Path(command[command.index("-o") + 1]).name == "xr_3da":
                link = command
    if link is None:
        parser.error("The build has no xr_3da link command")
    with tempfile.TemporaryDirectory(prefix="oxr-submit-queue-") as directory:
        output = Path(directory)
        object_file = str(output / "vulkan_submit_queue.o")
        executable = output / "vulkan_submit_queue"
        compile_command = shlex.split(entry["command"])
        compile_command[compile_command.index("-o") + 1] = object_file
        compile_command[compile_command.index("-c") + 1] = str(root / "misc/tests/vulkan_submit_queue.cpp")
        subprocess.run(compile_command, cwd=entry["directory"], check=True)
        entry_objects = [i for i, arg in enumerate(link) if "xr_3da.dir/" in arg and arg.endswith(".o")]
        if not entry_objects:
            parser.error("The link command has no xr_3da entry-point objects")
        link[link.index("-o") + 1] = str(executable)
        link[entry_objects[0]] = object_file
        for index in reversed(entry_objects[1:]):
            del link[index]
        subprocess.run(link, cwd=build, check=True)
        subprocess.run([str(executable)], cwd=output, check=True, timeout=20)


if __name__ == "__main__":
    main()
