"""Check the compiled detail shaders' descriptor and push-constant contract."""

import struct
import sys
import json
from pathlib import Path


def text(words):
    return struct.pack(f"<{len(words)}I", *words).split(b"\0", 1)[0].decode("utf-8")


def reflect(path):
    data = path.read_bytes()
    assert len(data) >= 20 and len(data) % 4 == 0, path
    words = struct.unpack(f"<{len(data) // 4}I", data)
    assert words[0] == 0x07230203, path
    names, bindings, sets, offsets = {}, {}, {}, {}
    samples = 0
    fragment = False
    cursor = 5
    while cursor < len(words):
        count, opcode = words[cursor] >> 16, words[cursor] & 0xFFFF
        assert count and cursor + count <= len(words), path
        args = words[cursor + 1:cursor + count]
        if opcode == 5:  # OpName
            names[args[0]] = text(args[1:])
        elif opcode == 15:  # OpEntryPoint
            fragment |= args[0] == 4 and text(args[2:]) == "main"
        elif opcode == 71:  # OpDecorate
            if args[1] == 33:
                bindings[args[0]] = args[2]
            elif args[1] == 34:
                sets[args[0]] = args[2]
        elif opcode == 72 and args[2] == 35:  # OpMemberDecorate Offset
            offsets[args[0], args[1]] = args[3]
        elif opcode in (87, 88):  # OpImageSampleImplicit/ExplicitLod
            samples += 1
        cursor += count
    assert fragment and samples >= 2, path
    slots = {name: (sets.get(identifier), bindings.get(identifier))
             for identifier, name in names.items() if identifier in bindings}
    scales = [offset for (identifier, member), offset in offsets.items()
              if names.get(identifier) == "MaterialParams" and member == 2]
    assert scales == [68], (path, scales)
    assert slots.get("diffuse") == (0, 0), (path, slots)
    assert slots.get("single_detail") == (0, 2), (path, slots)
    return slots


def main(root):
    shaders = root / "vk"
    for name, has_lightmap, forward in (
        ("level_detail_cutout", True, False),
        ("level_detail_unlit_cutout", False, False),
        ("level_detail_blended", True, True),
        ("level_detail_unlit_blended", False, True),
    ):
        slots = reflect(shaders / f"{name}.ps.spv")
        assert (slots.get("lightmap") == (0, 1)) == has_lightmap, (name, slots)
        assert ((1, 0) in slots.values()) == forward, (name, slots)
    for name in ("object_detail_opaque", "object_detail_cutout", "object_detail_blended",
                 "skinned_detail_opaque", "skinned_detail_cutout", "skinned_detail_blended",
                 "object_detail_double_sided_opaque", "object_detail_double_sided_cutout",
                 "object_detail_double_sided_blended"):
        slots = reflect(shaders / f"{name}.ps.spv")
        assert "lightmap" not in slots, (name, slots)
        assert ((2 if name.startswith("skinned_") else 1, 0) in slots.values()) == name.endswith("blended"), (name, slots)
    slots = reflect(shaders / "level_vegetation_detail_blended.ps.spv")
    assert (1, 0) in slots.values() and "lightmap" not in slots

    # Camera/material UV share a 256-byte range; static rotation is at 240.
    # Skinned transforms/hemi live in the pose SSBO, outside material data.
    manifest = json.loads((shaders / "opaque-variants.json").read_text())
    for variant in manifest["variants"]:
        if variant["source"] not in ("opaque.vs", "skinned.vs"):
            continue
        path = root / variant["output"]
        data = path.read_bytes()
        words = struct.unpack(f"<{len(data) // 4}I", data)
        names, offsets = {}, {}
        cursor = 5
        while cursor < len(words):
            count, opcode = words[cursor] >> 16, words[cursor] & 0xFFFF
            args = words[cursor + 1:cursor + count]
            if opcode == 5:
                names[args[0]] = text(args[1:])
            elif opcode == 72 and args[2] == 35:
                offsets[args[0], args[1]] = args[3]
            cursor += count
        camera = [offset for (identifier, member), offset in offsets.items()
                  if names.get(identifier) == "Camera"]
        expected = [0] if variant["source"] == "skinned.vs" else [0, 64, 128, 144, 160, 176, 192, 208, 224, 240]
        assert camera == expected, (path, camera)


if __name__ == "__main__":
    main(Path(sys.argv[1]))
