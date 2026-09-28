#!/usr/bin/env python3
"""Validate the Android GLSL compatibility layer with the NDK compiler.

OpenXRay expands shader includes itself and applies a small GLES source
translation before compilation. This tool mirrors both operations. Its default
manifest covers the shader programs that failed in the attached Adreno startup
log, including the vertex/fragment interfaces that a per-stage compiler alone
would miss.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


INCLUDE_RE = re.compile(r'#include\s+"([^"]+)"')
ERROR_RE = re.compile(r"<stdin>:(\d+):")
OUTPUT_RE = re.compile(
    r"^(?P<indent>\s*)out\s+(?:lowp\s+|mediump\s+|highp\s+)?"
    r"(?:vec4|float4)\s+(?P<name>SV_Target(?P<index>\d*))\s*;"
)
VARYING_RE = re.compile(
    r"^(?P<prefix>\s*layout\s*\([^)]*\blocation\b[^)]*\)\s*)"
    r"(?P<qualifiers>(?:(?:flat|smooth|noperspective|centroid|sample|"
    r"invariant|precise)\s+)*)"
    r"(?P<storage>in|out)\s+"
    r"(?P<precision>(?:lowp|mediump|highp)\s+)?"
    r"(?P<type>[A-Za-z_][A-Za-z0-9_]*)\s+"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
)
INTERFACE_RE = re.compile(
    r"layout\s*\(\s*location\s*=\s*(\d+)[^)]*\)\s*"
    r"(?:(?:flat|smooth|noperspective|centroid|sample|invariant|precise)\s+)*"
    r"(in|out)\s+(?:(?:lowp|mediump|highp)\s+)?"
    r"(vec[234]|ivec[234]|uvec[234]|float|int|uint)\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*;"
)
EXPLICIT_OUTPUT_RE = re.compile(
    r"layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*"
    r"out\s+(?:lowp\s+|mediump\s+|highp\s+)?(?:vec4|float4)\s+"
    r"SV_Target(\d*)\s*;"
)
REWRITE_RE = re.compile(
    r'^XR_GLES_LINE_REWRITE\(("(?:\\.|[^"\\])*"), ("(?:\\.|[^"\\])*"), '
    r'("(?:\\.|[^"\\])*")\)$'
)

STAGES = {".vs": "vert", ".ps": "frag", ".gs": "geom", ".cs": "comp"}
DEFAULT_PROGRAMS = (
    ("stub_notransform_aa_aa.vs", "accum_sun_nomsaa.ps"),
    ("accum_sun.vs", "accum_sun_near_nomsaa_nominmax.ps"),
    ("combine_1.vs", "combine_1_nomsaa.ps"),
    ("combine_1.vs", "combine_volumetric.ps"),
    ("deffer_particle.vs", "deffer_particle.ps"),
    ("model_def_lq.vs", "model_def_lq.ps"),
    ("sky2.vs", "sky2.ps"),
    ("water_soft.vs", "water_soft.ps"),
    ("waterd_soft.vs", "waterd_soft.ps"),
    # Android maps the GL renderer's depth-only "null" fragment stage to
    # dumb.ps because the Adreno monolithic GLES path requires both stages.
    ("shadow_direct_base.vs", "dumb.ps"),
    ("shadow_direct_model.vs", "dumb.ps"),
    ("shadow_direct_tree.vs", "dumb.ps"),
    ("shadow_direct_tree_s.vs", "dumb.ps"),
    ("shadow_direct_base_aref.vs", "shadow_direct_base_aref.ps"),
    ("shadow_direct_model_aref.vs", "shadow_direct_base_aref.ps"),
    ("shadow_direct_tree_aref.vs", "shadow_direct_base_aref.ps"),
    ("shadow_direct_tree_s_aref.vs", "shadow_direct_base_aref.ps"),
)
EXTRA_STARTUP_SHADERS = ("accum_sun_mask_nomsaa.ps", "yuv2rgb.ps")

# These values model the non-MSAA, advanced renderer selected in the device
# log. Unlike the old Cartesian profile sweep, this is a coherent runtime
# configuration and therefore does not manufacture impossible combinations.
RUNTIME_DEFINES = (
    "SMAP_size=2048",
    "SKIN_NONE=1",
    "USE_HWSMAP=1",
    "USE_HWSMAP_PCF=1",
    "USE_SJITTER=1",
    "USE_BRANCHING=1",
    "USE_VTF=1",
    "USE_TSHADOWS=1",
    "USE_MBLUR=1",
    "USE_SUNFILTER=1",
    "USE_SSAO_BLUR=1",
    "USE_SOFT_WATER=1",
    "USE_SOFT_PARTICLES=1",
    "USE_DOF=1",
    "ALLOW_STEEPPARALLAX=1",
    "GBUFFER_OPTIMIZATION=1",
    "SM_4_1=1",
    "SUN_SHAFTS_QUALITY=1",
    "SSAO_QUALITY=2",
    "SSAO_OPT_DATA=1",
    "SUN_QUALITY=2",
    "SSR_QUALITY=0",
    "MSAA_SAMPLES=0",
    "FXAA_360=0",
    "FXAA_PS3=0",
)


def load_line_rewrites() -> dict[tuple[str, str], str]:
    rules_path = (
        Path(__file__).resolve().parents[1]
        / "src/Layers/xrRenderPC_GL/AndroidGlslCompatRules.inl"
    )
    rules: dict[tuple[str, str], str] = {}
    for line_number, line in enumerate(rules_path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.startswith("XR_GLES_LINE_REWRITE"):
            continue
        match = REWRITE_RE.fullmatch(line)
        if not match:
            raise ValueError(f"malformed GLES rewrite at {rules_path}:{line_number}")
        source_path, source, replacement = (json.loads(value) for value in match.groups())
        key = (source_path, source)
        previous = rules.setdefault(key, replacement)
        if previous != replacement:
            raise ValueError(f"conflicting GLES rewrite for {source_path}:{source!r}")
    if not rules:
        raise ValueError(f"no GLES rewrite rules found in {rules_path}")
    return rules


LINE_REWRITES = load_line_rewrites()
NORMALIZED_LINE_REWRITES: dict[tuple[str, str], str] = {}
for (source_path, source), replacement in LINE_REWRITES.items():
    normalized = "".join(source.split())
    previous = NORMALIZED_LINE_REWRITES.setdefault((source_path, normalized), replacement)
    if "".join(previous.split()) != "".join(replacement.split()):
        raise ValueError(f"conflicting whitespace-normalized GLES rewrite for {source!r}")


def find_ndk_glslc(ndk: Path) -> tuple[Path, Path]:
    glslc = ndk / "shader-tools/linux-x86_64/glslc"
    libcxx = ndk / "toolchains/llvm/prebuilt/linux-x86_64/lib/x86_64-unknown-linux-gnu"
    if not glslc.is_file():
        raise FileNotFoundError(f"glslc not found: {glslc}")
    if not (libcxx / "libc++.so").is_file():
        raise FileNotFoundError(f"NDK host libc++ not found: {libcxx}")
    return glslc, libcxx


def resolve_include(shader_root: Path, include_name: str) -> Path:
    """Resolve includes like the engine's case-insensitive virtual FS."""
    current = shader_root
    for part in Path(include_name.replace("\\", "/")).parts:
        direct = current / part
        if direct.exists():
            current = direct
            continue
        folded = part.casefold()
        matches = [item for item in current.iterdir() if item.name.casefold() == folded]
        if len(matches) != 1:
            raise FileNotFoundError(f"cannot resolve shader include: {include_name}")
        current = matches[0]
    return current


def validate_rewrite_coverage(shader_root: Path) -> tuple[bool, str]:
    present: set[tuple[str, str]] = set()
    for path in shader_root.rglob("*"):
        if not path.is_file():
            continue
        try:
            source = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            source = path.read_text(encoding="windows-1251")
        source_path = path.relative_to(shader_root).as_posix()
        present.update((source_path, line.strip()) for line in source.splitlines())

    missing = sorted(key for key in LINE_REWRITES if key not in present)
    if missing:
        return False, "stale engine rewrite rules:\n" + "\n".join(
            f"{path}:{source!r}" for path, source in missing
        )
    return True, f"{len(LINE_REWRITES)} engine-source rewrite rules match upstream assets"


def physical_varying_type(type_name: str) -> tuple[str, str] | None:
    aliases = {
        "float": ("vec4", ".x"),
        "half": ("vec4", ".x"),
        "float2": ("vec4", ".xy"),
        "half2": ("vec4", ".xy"),
        "vec2": ("vec4", ".xy"),
        "float3": ("vec4", ".xyz"),
        "half3": ("vec4", ".xyz"),
        "vec3": ("vec4", ".xyz"),
        "int": ("ivec4", ".x"),
        "int2": ("ivec4", ".xy"),
        "ivec2": ("ivec4", ".xy"),
        "int3": ("ivec4", ".xyz"),
        "ivec3": ("ivec4", ".xyz"),
        "uint": ("uvec4", ".x"),
        "uint2": ("uvec4", ".xy"),
        "uvec2": ("uvec4", ".xy"),
        "uint3": ("uvec4", ".xyz"),
        "uvec3": ("uvec4", ".xyz"),
    }
    return aliases.get(type_name)


def transform_line(line: str, stage: str, source_path: str) -> list[str]:
    stripped = line.strip()
    replacement = LINE_REWRITES.get((source_path, stripped))
    if replacement is None:
        replacement = NORMALIZED_LINE_REWRITES.get(
            (source_path, "".join(stripped.split()))
        )
    if replacement is not None:
        indentation = line[: len(line) - len(line.lstrip(" \t\r"))]
        trailing = line[len(line.rstrip(" \t\r")) :]
        line = indentation + replacement + trailing
        stripped = line.strip()

    if stage == "frag" and stripped in {"in vec4 gl_FragCoord;", "in int gl_SampleID;"}:
        return [" " * len(line)]

    if stage == "frag" and "layout" not in line:
        output = OUTPUT_RE.match(line)
        if output:
            location = int(output.group("index") or "0")
            line = f"{output.group('indent')}layout(location = {location}) " + line[len(output.group("indent")) :]

    varying = VARYING_RE.match(line)
    wanted_storage = "out" if stage == "vert" else "in" if stage == "frag" else None
    if not varying or varying.group("storage") != wanted_storage:
        return [line]
    name = varying.group("name")
    if name.startswith("gl_") or name.startswith("_xray_gles_varying_"):
        return [line]
    physical = physical_varying_type(varying.group("type"))
    if not physical:
        return [line]

    physical_type, swizzle = physical
    physical_name = "_xray_gles_varying_" + name
    type_start, type_end = varying.span("type")
    name_start, name_end = varying.span("name")
    transformed = (
        line[:type_start]
        + physical_type
        + line[type_end:name_start]
        + physical_name
        + line[name_end:]
    )
    return [transformed, f"#define {name} {physical_name}{swizzle}"]


def expand_shader(path: Path, shader_root: Path, stage: str) -> tuple[str, list[tuple[Path, int]]]:
    output: list[str] = []
    locations: list[tuple[Path, int]] = []

    def append(source_path: Path) -> None:
        try:
            source = source_path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            source = source_path.read_text(encoding="windows-1251")

        virtual_path = source_path.relative_to(shader_root).as_posix()
        for line_number, line in enumerate(source.splitlines(), start=1):
            include = INCLUDE_RE.search(line)
            if include:
                prefix = line[: include.start()]
                if prefix:
                    for transformed in transform_line(prefix, stage, virtual_path):
                        output.append(transformed)
                        locations.append((source_path, line_number))
                append(resolve_include(shader_root, include.group(1)))
                suffix = line[include.end() :]
                if suffix:
                    for transformed in transform_line(suffix, stage, virtual_path):
                        output.append(transformed)
                        locations.append((source_path, line_number))
                continue

            for transformed in transform_line(line, stage, virtual_path):
                output.append(transformed)
                locations.append((source_path, line_number))

    append(path)
    return "\n".join(output) + "\n", locations


def build_source(shader: Path, shader_root: Path) -> tuple[str, list[tuple[Path, int]], int]:
    stage = STAGES[shader.suffix]
    expanded, locations = expand_shader(shader, shader_root, stage)
    define_values: dict[str, str] = {}
    for define in RUNTIME_DEFINES:
        name, separator, value = define.partition("=")
        define_values[name] = value if separator else "1"
    header_lines = [
        "#version 320 es",
        "precision highp float;",
        "precision highp int;",
        "precision lowp sampler3D;",
        "precision lowp sampler2DMS;",
        "precision lowp sampler2DShadow;",
        *(f"#define {name} {value}" for name, value in define_values.items()),
        "#if defined(SKIN_NONE) || defined(SKIN_0)",
        "#define skin_input_normal(value) value.xyz",
        "#else",
        "#define skin_input_normal(value) value",
        "#endif",
        "#if defined(SKIN_NONE) || defined(SKIN_0) || defined(SKIN_1) || defined(SKIN_2)",
        "#define skin_input_tangent(value) value.xyz",
        "#else",
        "#define skin_input_tangent(value) value",
        "#endif",
        "#if defined(SKIN_2) || defined(SKIN_3)",
        "#define skin_input_tc(value) value",
        "#else",
        "#define skin_input_tc(value) value.xy",
        "#endif",
    ]
    return "\n".join(header_lines) + "\n" + expanded, locations, len(header_lines)


def compiler_env(libcxx: Path) -> dict[str, str]:
    env = os.environ.copy()
    previous = env.get("LD_LIBRARY_PATH")
    env["LD_LIBRARY_PATH"] = str(libcxx) if not previous else f"{libcxx}{os.pathsep}{previous}"
    return env


def invoke_glslc(glslc: Path, libcxx: Path, shader: Path, source: str, preprocess: bool = False) -> subprocess.CompletedProcess[str]:
    command = [
        str(glslc),
        f"-fshader-stage={STAGES[shader.suffix]}",
        "-std=320es",
        "--target-env=opengl",
        "-x",
        "glsl",
    ]
    if preprocess:
        command.extend(["-E", "-"])
    else:
        command.extend(
            ["-fauto-bind-uniforms", "-fauto-map-locations", "-", "-o", os.devnull]
        )
    return subprocess.run(
        command,
        input=source,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=compiler_env(libcxx),
        check=False,
    )


def format_diagnostics(output: str, locations: list[tuple[Path, int]], header_size: int, shader_root: Path) -> str:
    messages: list[str] = []
    for message in output.splitlines():
        match = ERROR_RE.search(message)
        if match:
            expanded_line = int(match.group(1)) - header_size
            if 1 <= expanded_line <= len(locations):
                source_path, source_line = locations[expanded_line - 1]
                message += f" [{source_path.relative_to(shader_root)}:{source_line}]"
        messages.append(message)
    return "\n".join(messages)


def compile_shader(glslc: Path, libcxx: Path, shader_root: Path, shader: Path) -> tuple[bool, str]:
    source, locations, header_size = build_source(shader, shader_root)
    if not re.search(r"\bvoid\s+main\s*\(", source):
        return False, "expanded source has no main()"
    if shader.suffix == ".ps":
        targets = re.findall(r"\bout\s+(?:lowp\s+|mediump\s+|highp\s+)?(?:vec4|float4)\s+SV_Target(\d*)\s*;", source)
        explicit_targets = {
            suffix: location for location, suffix in EXPLICIT_OUTPUT_RE.findall(source)
        }
        for suffix in targets:
            expected = suffix or "0"
            if explicit_targets.get(suffix) != expected:
                return False, f"SV_Target{suffix} has no explicit layout(location = {expected})"
    result = invoke_glslc(glslc, libcxx, shader, source)
    return result.returncode == 0, format_diagnostics(result.stdout, locations, header_size, shader_root)


def compile_skinning_variant(
    glslc: Path, libcxx: Path, shader_root: Path, shader: Path, skin: int
) -> tuple[bool, str]:
    source, locations, header_size = build_source(shader, shader_root)
    replacements = source.count("#define SKIN_NONE 1")
    if replacements != 1:
        return False, "validator runtime profile has no unique SKIN_NONE definition"
    source = source.replace("#define SKIN_NONE 1", f"#define SKIN_{skin} 1", 1)
    result = invoke_glslc(glslc, libcxx, shader, source)
    return result.returncode == 0, format_diagnostics(
        result.stdout, locations, header_size, shader_root
    )


def shader_interface(glslc: Path, libcxx: Path, shader_root: Path, shader: Path) -> tuple[dict[int, tuple[str, str]], str]:
    source, _, _ = build_source(shader, shader_root)
    result = invoke_glslc(glslc, libcxx, shader, source, preprocess=True)
    if result.returncode != 0:
        return {}, result.stdout
    wanted = "out" if shader.suffix == ".vs" else "in"
    interface: dict[int, tuple[str, str]] = {}
    for match in INTERFACE_RE.finditer(result.stdout):
        location, storage, type_name, name = match.groups()
        if storage == wanted:
            interface[int(location)] = (type_name, name)
    return interface, ""


def validate_program_interface(
    glslc: Path, libcxx: Path, shader_root: Path, vertex: Path, fragment: Path
) -> tuple[bool, str]:
    vertex_outputs, vertex_error = shader_interface(glslc, libcxx, shader_root, vertex)
    fragment_inputs, fragment_error = shader_interface(glslc, libcxx, shader_root, fragment)
    if vertex_error or fragment_error:
        return False, vertex_error + fragment_error

    problems: list[str] = []
    for location, (fragment_type, fragment_name) in fragment_inputs.items():
        vertex_value = vertex_outputs.get(location)
        if vertex_value is None:
            problems.append(f"location {location}: fragment input {fragment_name} has no vertex output")
        elif vertex_value[0] != fragment_type:
            problems.append(
                f"location {location}: vertex {vertex_value[0]} != fragment {fragment_type} ({fragment_name})"
            )
    return not problems, "\n".join(problems)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ndk", required=True, type=Path)
    parser.add_argument("--shader-root", type=Path, default=Path("res/gamedata/shaders/gl"))
    parser.add_argument("shaders", nargs="*")
    args = parser.parse_args()

    shader_root = args.shader_root.resolve()
    glslc, libcxx = find_ndk_glslc(args.ndk.resolve())

    if args.shaders:
        shaders = [(shader_root / item).resolve() for item in args.shaders]
        programs: tuple[tuple[str, str], ...] = ()
    else:
        names = {name for pair in DEFAULT_PROGRAMS for name in pair}
        names.update(EXTRA_STARTUP_SHADERS)
        shaders = [(shader_root / name).resolve() for name in sorted(names)]
        programs = DEFAULT_PROGRAMS

    failures = 0
    default_shader_root = (Path(__file__).resolve().parents[1] / "res/gamedata/shaders/gl").resolve()
    if not args.shaders and shader_root == default_shader_root:
        ok, diagnostics = validate_rewrite_coverage(shader_root)
        if ok:
            print(f"PASS rules {diagnostics}")
        else:
            failures += 1
            print(f"FAIL rules\n{diagnostics}")

    for shader in shaders:
        ok, diagnostics = compile_shader(glslc, libcxx, shader_root, shader)
        if ok:
            print(f"PASS stage {shader.name}")
        else:
            failures += 1
            print(f"FAIL stage {shader.name}\n{diagnostics}")

    for vertex_name, fragment_name in programs:
        vertex = (shader_root / vertex_name).resolve()
        fragment = (shader_root / fragment_name).resolve()
        ok, diagnostics = validate_program_interface(
            glslc, libcxx, shader_root, vertex, fragment
        )
        label = f"{vertex_name} + {fragment_name}"
        if ok:
            print(f"PASS link  {label}")
        else:
            failures += 1
            print(f"FAIL link  {label}\n{diagnostics}")

    skinning_checks = 0
    if not args.shaders:
        skinning_shader = (shader_root / "model_def_lq.vs").resolve()
        for skin in range(5):
            skinning_checks += 1
            ok, diagnostics = compile_skinning_variant(
                glslc, libcxx, shader_root, skinning_shader, skin
            )
            if ok:
                print(f"PASS skin  model_def_lq.vs SKIN_{skin}")
            else:
                failures += 1
                print(f"FAIL skin  model_def_lq.vs SKIN_{skin}\n{diagnostics}")

    print(
        f"Validated {len(shaders)} stages, {len(programs)} interfaces and "
        f"{skinning_checks} skinning variants; failures: {failures}"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
